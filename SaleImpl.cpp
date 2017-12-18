/**
 * Description:
 *		It is the main class of sale implemention.
 *		it contains the sale operations and data, it's the "model" of Sale..
 *		it includes the following main functions:
 *			Add new Sale Item:  AddItem
 *			Modify a sale item's quantity or price: ModifyQuantity, ModifyPrice
 *			Cancel a sale item: CancelItem
 *			Save a Sale bill:	Save
 *
 *		it also includes some auxiliary functions:
*			HangupSale, UnHangupSale etc. 
 *
 *
 * history
 *   1, create				2002-07-15
 *   2, modified			2002-09-11
 *
 * author
 *		kevin chen
 */ 
#include "stdafx.h"
#include "pos.h"
#include "SaleImpl.h"
#include "AccessDataBase.h"
#include "PosClientDB.h"
#include "GlobalFunctions.h"
#include <algorithm>
#include "MemberImpl.h"
#include  <math.h>
#include "CouponImpl.h"
#include "ConfirmLimitSaleDlg.h"
#include "../dll/unitools/UniTools.h"
#include "UnHangUpDlg.h"
#include "AllBackDlg.h"
#include "InvoiceNOSegment.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

//##ModelId=3E641FAA025C
CSaleImpl::CSaleImpl() : m_nSaleID(1),m_stratege_xiangou(this)//,m_EndSaleMerchInfo(CString("inputplu"))2013-2-20 17:33:11 退出销售崩溃
{	m_bReprint=false;
	m_IsLoadedUncompletedSale = false;
	m_strategy_WholeBill_Stamp=NULL;
	Clear();
	for (int i = 0; i<10; i++)
	{
		StyleID[i] = 0;
	}

	m_dbSumMemDis = 0.0f; //会员折扣总金额
	m_dbSumPromDis = 0.0f;//促销活动总金额

	m_strInvoiceCode = _T("");
	m_strInvoiceNoFirst = _T("");
	m_strInvoiceStartNo = _T("");
	m_strInvoiceEndNo = _T("");	
	m_nSegmentID = 0;
	m_nInvoiceTotalPrintLineSale = 0; 
	m_nInvoiceTotalPageSale = 0;
	m_nInvoiceSaleMerchLineEnd = 0;
	m_nInvoiceSaleMerchLineStart = 0;
	m_nInvoicePaymentLineEnd = 0;
	m_nInvoicePaymentLineStart = 0;
	m_nInvoiceAmtTotalLineEnd = 0;
	m_nInvoiceAmtTotalLineStart = 0;
	m_nInvoiceHeadLine = 0;
	m_bHasPrepaidCardPS = false;
	m_eInvoiceRemain = INVOICE_REMAIN_GREATER;
	m_strPromoterID = _T("");
}

//##ModelId=3E641FAA02B6
CSaleImpl::~CSaleImpl() {
	Clear();
	//EmptyTempSale();
}

//清除本笔销售
//##ModelId=3E641FAC00F7
void CSaleImpl::Clear()
{
	//EndMerchClear(); //清空最后商品记录
	if(m_strategy_WholeBill_Stamp){
		delete m_strategy_WholeBill_Stamp;
		m_strategy_WholeBill_Stamp=NULL;
	}//★★★ 2012-9-24 17:29:34 proj 2012-9-10 R2POS T1987 总单印花 1.整单印花活动范围=2，第一笔交易会员，给5个印花，第二笔交易非会员，也报给5个印花
	m_strategy_WholeBill_Stamp=new CStrategy_WholeBill_Stamp();
	m_strategy_WholeBill_Stamp->SetSaleImpl(this);
	m_stratege_xiangou.clear();
	m_PayImpl.Clear();
	//m_MemberImpl.Clear();
	m_strCustomerCharacter = "";
	m_nDiscountStyle = SDS_NO;
	m_nMemDiscountStyle = SDS_NO;
	m_fDiscount = 0.0;
	m_fMemDiscount = 0.0;
	m_fDiscountRate = 100.0;
	m_fMemDiscountRate = 100.0;
	m_bCanChangePrice = false;
	m_bFixedPriceAndQuantity = false;
	IsDiscountLabelBar = false;
	m_IsLoadedUncompletedSale=false;

	ClearCashier(m_tSalesman);
	m_PayImpl.SetSaleImpl(this);
	//m_bMember = false;/2013-3-4 15:05:20 proj2013-3-4-1 会员优化
	GetSalePos()->GetPromotionInterface()->m_vreloadPromotion.clear();//aori add ★★★ 2012-8-14 15:40:37 proj 2012-8-14-1 促销信息错误
	//初始电商变量
	//m_EorderID = "";
	//m_EMemberID = "";
	//m_EMOrderStatus = 0;

	m_nInvoiceTotalPrintLinePrepaid = 0;
	m_nInvoiceTotalPagePrepaid = 0;
	m_nInvoiceTotalPrintLineAPay = 0;
	m_nInvoiceTotalPageAPay= 0;
	m_nInvoiceTotalPage = 0;
	m_vecPayItemAPay.clear();
}
double CSaleImpl::GetBNQActualTotalAmt()
{
	double dTotal = 0;
	for ( vector<SaleMerchInfo>::const_iterator merchInfo = m_vecSaleMerch.begin();
	merchInfo != m_vecSaleMerch.end(); ++merchInfo ) 
	{			
		if ( merchInfo->nMerchState != RS_CANCEL )									
			dTotal += merchInfo->fActualPrice * merchInfo->fSaleQuantity/*nSaleCount*/;
	}
	ReducePrecision(dTotal);
	return dTotal;
}
//从链表中取合计信息
//##ModelId=3E641FAC009D
double CSaleImpl::GetTotalAmt(bool bWithoutPromotion)
{
	//CString strLog;
	//CUniTrace::Trace("Fun Begin->CSaleImpl::GetTotalAmt");
	double dTotal = 0;
	for ( vector<SaleMerchInfo>::const_iterator merchInfo = m_vecSaleMerch.begin();merchInfo != m_vecSaleMerch.end(); ++merchInfo ) 
	{			
		if ( merchInfo->nMerchState != RS_CANCEL && (!bWithoutPromotion|| (bWithoutPromotion && merchInfo->nMerchState == RS_NORMAL )))									
				dTotal += merchInfo->fSaleAmt;
	}
   //减去促销折扣金额
    dTotal-=GetSalePos()->GetPromotionInterface()->m_fDisCountAmt;
	ReducePrecision(dTotal);
	
/*	//从数据库查

	double dTotal_db = GetSaleTotAmtFromDB();
	strLog.Format("从SaleItem_temp取得金额:%f",dTotal_db);
	CUniTrace::Trace(strLog);
	strLog.Format("Fun End->CSaleImpl::GetTotalAmt->返回值:%f",dTotal);
	CUniTrace::Trace(strLog);*/
	return dTotal;
}

//计算合计金额，指定商品明细集合(不扣减条件促销金额)
double CSaleImpl::GetTotalAmt(const vector<SaleMerchInfo> *pVecSaleMerch,bool bWithoutPromotion)
{
	//CString strLog;
	//CUniTrace::Trace("Fun Begin->CSaleImpl::GetTotalAmt");
	double dTotal = 0;
	for ( vector<SaleMerchInfo>::const_iterator merchInfo = pVecSaleMerch->begin();merchInfo != pVecSaleMerch->end(); ++merchInfo ) 
	{			
		if ( merchInfo->nMerchState != RS_CANCEL && (!bWithoutPromotion|| (bWithoutPromotion && merchInfo->nMerchState == RS_NORMAL )))									
				dTotal += merchInfo->fSaleAmt;
	}
 
   //减去促销折扣金额
	/*
    dTotal-=GetSalePos()->GetPromotionInterface()->m_fDisCountAmt;
	ReducePrecision(dTotal);*/
	return dTotal;
}

double CSaleImpl::GetTotalAmt_FORYINHUA(bool bWithoutPromotion)
{
	double dTotal = 0;
	for ( vector<SaleMerchInfo>::const_iterator merchInfo = m_vecSaleMerch.begin();merchInfo != m_vecSaleMerch.end(); ++merchInfo ) {			
		if ( merchInfo->nMerchState != RS_CANCEL 
			&& (!bWithoutPromotion|| (bWithoutPromotion && merchInfo->nMerchState == RS_NORMAL )))									
				dTotal += merchInfo->fSaleAmt;
	}
	
	return dTotal;
}


double CSaleImpl::GetTotalAmt_beforePromotion(bool bWithoutPromotion) 
{
	double dTotal = 0;
	for ( vector<SaleMerchInfo>::const_iterator merchInfo = m_vecSaleMerch.begin();
		merchInfo != m_vecSaleMerch.end(); ++merchInfo ) {			
			if ( merchInfo->nMerchState != RS_CANCEL && (!bWithoutPromotion
				|| (bWithoutPromotion && merchInfo->nMerchState == RS_NORMAL )))									
				dTotal += merchInfo->fSaleAmt_BeforePromotion;
	}
   //减去促销折扣金额
    dTotal-=GetSalePos()->GetPromotionInterface()->m_fDisCountAmt;//SumPromotionAmt(m_nSaleID);//aori replace m_fDisCountAmt to sum 2012-2-9 13:49:53 proj 2.79 测试反馈：重打印小票 应付金额错误;//aori del 2012-2-7 16:11:04 proj 2.79 测试反馈：条件促销时 不找零 加 fSaleAmt_BeforePromotion
	ReducePrecision(dTotal);
	return dTotal;
}


double CSaleImpl::GetTotalAmt_beforePromotion(const vector<SaleMerchInfo> *pVecSaleMerch,bool bWithoutPromotion)
{
	double dTotal = 0;
	for ( vector<SaleMerchInfo>::const_iterator merchInfo = pVecSaleMerch->begin();
		merchInfo != pVecSaleMerch->end(); ++merchInfo ) {			
			if ( merchInfo->nMerchState != RS_CANCEL && (!bWithoutPromotion
				|| (bWithoutPromotion && merchInfo->nMerchState == RS_NORMAL )))									
				dTotal += merchInfo->fSaleAmt_BeforePromotion;
	}
	return dTotal;
}
 

double CSaleImpl::GetBNQTotalAmt()
{
	double dTotal = 0;
	for ( vector<SaleMerchInfo>::const_iterator merchInfo = m_vecSaleMerch.begin();
	merchInfo != m_vecSaleMerch.end(); ++merchInfo ) 
	{			
		if ( merchInfo->nMerchState != RS_CANCEL )
		{
			dTotal += merchInfo->fSaleAmt;
		}
	}
	ReducePrecision(dTotal);

	double dDisCount = 0;
	GetSalePos()->GetPromotionInterface()->SumBNQPromotionAmt(PROMO_STYLE_CARD,dDisCount);
	m_dbSumMemDis = dDisCount;
	dTotal-=dDisCount;

	dDisCount = 0;
	GetSalePos()->GetPromotionInterface()->SumBNQPromotionAmt(PROMO_STYLE_CONDDISCOUNT,dDisCount);
	m_dbSumPromDis = dDisCount;
	dTotal-=dDisCount;

	ReducePrecision(dTotal);

	return dTotal;
}


//取消指定序号的销售
//##ModelId=3E641FAB03E4
bool CSaleImpl::CancelItem(int nSID)
{
	if ( nSID < m_vecSaleMerch.size() ) {
		m_vecSaleMerch[nSID].nMerchState = RS_CANCEL;
		//zenghl 20090608 WriteSaleItemLog
		WriteSaleItemLog(m_vecSaleMerch[nSID],1,0);	//aori add 2011-5-13 10:53:09 proj2011-5-11-2：销售不平5-9北科大3: 日志级别 writelog写成dll
		//add lxg.03/3/26修改临时商品表
		UpdateItemInTemp(nSID);

	}
	return true;
}

//取消指定序号的销售
//##ModelId=3E641FAB03E4
bool CSaleImpl::CancelItem(int nSID,char* authorizePerson)
{
	if ( nSID < m_vecSaleMerch.size()&&m_vecSaleMerch[nSID].nMerchState!=RS_CANCEL ) {//aori add 测试反馈：2012-2-24 14:47:19  proj 2012-2-13 折扣标签校验 ：全取消时多减了一个。
		SaleMerchInfo* pCancelMerch=&m_vecSaleMerch[nSID];//aori add for debuginfo
		m_vecSaleMerch[nSID].nMerchState = RS_CANCEL;
		m_vecSaleMerch[nSID].szAuthorizePerson=authorizePerson;
		//zenghl 20090608 WriteSaleItemLog
		WriteSaleItemLog(m_vecSaleMerch[nSID],1,0);	//aori add 2011-5-13 10:53:09 proj2011-5-11-2：销售不平5-9北科大3: 日志级别 writelog写成dll
		m_stratege_xiangou.UpdatelimitInfo(m_vecSaleMerch[nSID]);
		//add lxg.03/3/26修改临时商品表
		UpdateItemInTemp(nSID);
		//if (m_vecSaleMerch[nSID].LimitStyle<LIMITSTYLE_UPLIMIT&&m_vecSaleMerch[nSID].LimitStyle>LIMITSTYLE_NoLimit){//aori add2011-4-11 11:36:20 bug: 换会员号后删商品后加同一商品,限购错误 :
			//ReCalculateMemPrice(false);////aori add bNeedZeroLimitInfo 2011-4-8 14:33:28 limit bug 取消商品后 限购错误
		ReCalculateMemPrice(true);//aori change 2011-4-11 11:36:20 bug: 换会员号后删商品后加同一商品,限购错误

		CSaleItem hh(m_vecSaleMerch[nSID].szPLUInput);
		if(hh.m_bIsDiscountLabelBar)
			GetSalePos()->GetPOSClientImpl()->checkDiscountLabel((char*)(LPCSTR)(m_vecSaleMerch[nSID].szPLUInput),false);//aori add 2012-2-16 14:45:33 proj 2012-2-13 折扣标签校验 加消息 ETI_DOWNLOAD_CHECK_DiscountLabel

	}
	return true;
}

void CSaleImpl::CancelOrderItem(int nSID,char* authorizePerson)
{
	if ( nSID < m_vecSaleMerch.size()&&m_vecSaleMerch[nSID].nMerchState!=RS_CANCEL )
	{
		CString strOrderNo = m_vecSaleMerch[nSID].strOrderNo;//得到订单号
		CString strPrompt;
		strPrompt.Format("是否要删除订单%s中的所有商品？", strOrderNo);
		if(AfxMessageBox(strPrompt, MB_ICONQUESTION | MB_OKCANCEL) == IDOK)
		{
			for (int n = 0 ; n < (int)m_vecSaleMerch.size(); n++) 
			{
				if(m_vecSaleMerch[n].nItemType == 2 && m_vecSaleMerch[n].strOrderNo == strOrderNo)
					CancelItem(n, authorizePerson);	
			}
			CFinDataManage finDataManage;
			finDataManage.UpdateOrderStatue(strOrderNo,ORDER_UNCHARGE);//设置成未收费
		}
	}
}

//挂起当前销售
//##ModelId=3E641FAB0344
bool CSaleImpl::HangupSale() {

	GetLocalTimeWithSmallTime(&m_stEndSaleDT);
	m_stCheckSaleDT = m_stEndSaleDT;

	CPosClientDB *pDB = CDataBaseManager::getConnection(true);
	if ( !pDB ) {
		return false;
	}

	pDB->BeginTrans();

	if ( !SaveHangup(pDB, m_nSaleID, SS_HANGUP) ) {
		pDB->RollbackTrans();
		CDataBaseManager::closeConnection(pDB);
		return false;
	}

	pDB->CommitTrans();
	CDataBaseManager::closeConnection(pDB);

	CString strBuf;
	strBuf.Format("%d", m_nSaleID);
	m_pADB->SetOperateRecord(OPT_HANGUP, GetSalePos()->GetCasher()->GetUserID(), strBuf);
	GetSalePos()->m_Task.putOperateRecord(m_stEndSaleDT);

	return true;
}

//
//	保存销售信息，两个地方可能用到，一是正常的销售，二是挂起销售
//	同时清除掉在临时表中的数据
//
//======================================================================
// 函 数 名：Save
// 功能描述：保存销售信息
// 输入参数：CPosClientDB* pDB, int nSaleID, unsigned char nSaleStatus
// 输出参数：bool
// 创建日期：00-2-21
// 修改日期：2009-4-1 曾海林
//修改日期20090526 修改为每次保存数据时调用LoadSaleInfo(
// 作  者：***
// 附加说明：数据库变更修改 SaleItem ,Sale
//======================================================================
bool CSaleImpl::Save(CPosClientDB* pDB, int nSaleID, unsigned char nSaleStatus)
{	
	GetSalePos()->GetPromotionInterface()->SetSaleImpl(this);//aori move from down 2011-9-6 17:57:52 优化CSaleImpl::Save
	double fTotDiscount = 0;//GetSalePos()->GetPromotionInterface()->m_fDisCountAmt;
	double fTotAmt = 0.0f, dTemp = 0.0f, fTempMemdiscount = 0.0f;
	double dManualDiscount = 0;
	CString strSQL, strManualDiscount, strSaleDT = ::GetFormatDTString(m_stEndSaleDT);//修改24小时销售的问题，将saledt修改为开票的时间//	CString strSaleDT = ::GetFormatDTString(m_stSaleDT);
	const int nUserID = GetSalePos()->GetCasher()->GetUserID();
	const char *szTableDTExt = GetSalePos()->GetTableDTExt(&m_stEndSaleDT);
	try	{//fTotDiscount=GetSalePos()->GetPromotionInterface()->m_fDisCountAmt;GetSalePos()->GetPromotionInterface()->SetSaleImpl(this);
		
		CUniTrace::Trace("!!!最终保存前一刻 各表内容");//aori add 2011-8-3 17:03:07 proj2011-8-3-1 销售不平 条件促销 临时表 记日志里 //GetSalePos()->GetPromotionInterface()->WritePromotionLog( nSaleID);//aori add 2011-8-3 17:03:07 proj2011-8-3-1 销售不平 条件促销 临时表 记日志里 
		
		GetSalePos()->GetPromotionInterface()->LoadSaleInfo(nSaleID,true);//20090526 修改为每次保存数据时调用LoadSaleInfo(//保存促销结果
	  	
		for ( vector<SaleMerchInfo>::iterator merchInfo = m_vecSaleMerch.begin(); merchInfo != m_vecSaleMerch.end(); ++merchInfo ) {
			if ( merchInfo->nMerchState != RS_CANCEL ) {//fTotDiscount += merchInfo->fSaleDiscount;//以后tot里面不加会员的了 + merchInfo->fMemSaleDiscount;
				fTotAmt += merchInfo->fSaleAmt;//保存到销售主表//计算销售折扣总计 //ReSortSaleMerch(); zenghl 20090528
				//fTempMemdiscount += merchInfo->fMemSaleDiscount;
				fTotDiscount += merchInfo->dCondPromDiscount;//条件促销
				dManualDiscount += merchInfo->fManualDiscount;
				fTempMemdiscount += merchInfo->dMemDiscAfterProm;//会员
			}
		}
		m_strategy_WholeBill_Stamp->GeneralYinHuaInfo_forBill();//aori add ★★★ 2012-9-11 17:06:26 proj 2012-9-10 R2POS T1987 总单印花

		string strCC;
		if ( m_strCustomerCharacter.size() > 0 ) {
			strCC = "'";strCC += m_strCustomerCharacter;strCC += "'";
		} else
			strCC = "NULL";
		CTime tmSaleDT(m_stSaleDT), tmEndSaleDT(m_stEndSaleDT), tmCheckSaleDT(m_stCheckSaleDT);
		int nEndSaleSpan = (tmEndSaleDT - tmSaleDT).GetTotalSeconds();
		if (nEndSaleSpan < 0 )//小于0数据处理
			nEndSaleSpan = 15;
		int nCheckSaleSpan = (tmCheckSaleDT - tmSaleDT).GetTotalSeconds();
		if (nCheckSaleSpan < 0 )//小于0数据处理
			nCheckSaleSpan = 10;

		ReducePrecision(fTotDiscount);//折扣取整到分
		//会员input
		//cardtype=0;
		//MemberCode = "";
		//Get_Member_input();
		//保存销售
		//2015-3-6增加电商支付字段AddCol6
		//2015-3-23增加电商状态字段Addcol8
		//百安居版本 addcol6字段存成会员 id=会员姓名
		CString strAddCol6 = "";
		CString strMemCode = "";
		strMemCode.Format("%s", m_MemberImpl.m_Member.szInputNO);
		if(!strMemCode.IsEmpty())
		{
			strAddCol6.Format("%d=%s", m_MemberImpl.m_Member.nHumanID, m_MemberImpl.m_Member.szMemberName);
			//30位截断
			if (strAddCol6.GetLength() > 30)
			{
				int i = 0;
				for(;i < 28;)	
				{
					unsigned char c = strAddCol6[i];
					if(strAddCol6[i] & 0x80)
					{
						i+=2;
					}
					else
					{
						i++;
					}
					if(i >= 28)
						break;
				}		
				strAddCol6 = strAddCol6.Left(i);
			}
		}

		strSQL.Format("INSERT INTO Sale%s(SaleDT,EndSaleDT,CheckSaleDT,SaleID,HumanID,TotAmt,TotDiscount,Status,PDID,MemberCode,CharCode,ManualDiscount,MemDiscount,UploadFlag,MemIntegral,Shift_No,stampcount,addcol5,MemCardNO,MemCardNOType,AddCol6,AddCol7,AddCol8," 
			          " MemberCardType,Ecardno,userID,MemberCardLevel,MemberCardchannel,PromoterID  ) "//★★★ 2012-9-14 16:07:35 proj 2012-9-10 R2POS T1987 总单印花 保存
				"VALUES ('%s',%d,%d,%ld,%ld,%.4f,%.4f,%d,%d,'%s',%s,%.4f,%.4f,0,%f,%d,%d,%d ,'%s',%d,'%s','%s','%d' ,'%s','%s','%s','%s','%s','%s')", 
				szTableDTExt, strSaleDT, nEndSaleSpan, nCheckSaleSpan, 
				nSaleID, nUserID, fTotAmt, fTotDiscount, 
				nSaleStatus, GetSalePos()->GetCurrentTimePeriod(),  m_MemberImpl.m_Member.szMemberCode,//GetFieldStringOrNULL(m_MemberImpl.m_Member.szMemberCode), 
				strCC.c_str(), /*m_fDiscount*/dManualDiscount, fTempMemdiscount,m_MemberImpl.m_dMemIntegral,m_uShiftNo,
						/*m_strategy_WholeBill_Stamp->m_YinHuaCount*/0,//百安居没有这个业务，这个字段应该填0，否则溢出[dandan.wu 2016-3-10]
						m_strategy_WholeBill_Stamp->m_StampGivingMinAmt,
						m_MemberImpl.m_Member.szInputNO,
						m_MemberImpl.m_Member.nCardNoType,strAddCol6/*m_EorderID*/,m_EMemberID,m_EMOrderStatus,
						m_MemberImpl.m_Member.szCardTypeCode,m_MemberImpl.m_Member.szEcardno,m_MemberImpl.m_Member.szUserID,
						m_MemberImpl.m_Member.szMemberCardLevel,m_MemberImpl.m_Member.szMemberCardchannel,m_strPromoterID);//把会员的整单打折也算到里面去m_fMemDiscount); //会员平台
		if ( pDB->Execute2(strSQL) == 0){//aori change 2011-3-22 16:51:36 优化clientdb ：删除execute2
			CString hh;hh.Format("!!!checkoutDLG::oncheck保存到sale%s时失败",szTableDTExt);CUniTrace::Trace(hh);return false;
		}
		//保存销售明细
		int nItemID = 0;
		bool bHasCanceledItems = false;
		double fSaleQuantity = 0.0f,fAddDiscount = GetItemDiscount();
		for ( merchInfo = m_vecSaleMerch.begin(); merchInfo != m_vecSaleMerch.end(); ++merchInfo ) {
			if ( merchInfo->nMerchState == RS_CANCEL ) {//取消商品保存在操作记录Remark里, FunID = 1 --- 取消的商品
				Sleep(5);//延迟1毫秒执行，防止时间主键重复
				m_pADB->SetOperateRecord(OPT_CANCELITEM, nUserID, merchInfo->szPLU);
				merchInfo->nSID=merchInfo-m_vecSaleMerch.begin();merchInfo->SetCancelSaleItem(m_stEndSaleDT, nSaleID, nUserID,1,(char*)(LPCSTR)merchInfo->szAuthorizePerson );
				bHasCanceledItems = true;
			} else {
				nItemID++;
				double fActualPrice = 0.0f;
				merchInfo->OutPut_Quantity_Price(fSaleQuantity, fActualPrice);
				if ( merchInfo->nMerchState == RS_DS_MANUAL ) {//如果是手动打折，打印出手动打折情况
					if ( merchInfo->nLowLimit == SDS_RATE ) {
// 						if(merchInfo->bFixedPriceAndQuantity)
// 						{
// 							dTemp = (merchInfo->fActualPrice * merchInfo->fSaleQuantity) * (100.0f - merchInfo->fZKPrice) / 100.0f;
// 						}
// 						else	
// 						{
// 							dTemp = (merchInfo->fActualPrice * merchInfo->nSaleCount) * (100.0f - merchInfo->fZKPrice) / 100.0f;
// 						}
						//数量真正存放在fSaleQuantity中 [dandan.wu 2016-4-26]
						dTemp = (merchInfo->fActualPrice * merchInfo->fSaleQuantity) * (100.0f - merchInfo->fZKPrice) / 100.0f;
						ReducePrecision(dTemp);
					} else {
						dTemp = merchInfo->fZKPrice;
					}
					strManualDiscount.Format("%.4f", dTemp);
				} else {
					strManualDiscount = "NULL";
				}//如果商品属于促销但是又没有到促销的限制范围内，重新设置商品促销标志
				if(merchInfo->nMerchState == RS_NORMAL && merchInfo->nDiscountStyle != DS_NO)
				{
					merchInfo->nDiscountStyle = DS_NO;//merchInfo->nMakeOrgNO = 0;//merchInfo->nSalePromoID = 0;
				}
				double fTempSaleDiscount = merchInfo->fSaleDiscount;//折扣取整到分
				if(fabs(ReducePrecision(fTempSaleDiscount)) > 0)
				{
					fTempSaleDiscount += fAddDiscount;
					fAddDiscount = 0.0f;
				}
				if(merchInfo->bIsDiscountLabel )
					merchInfo->fBackDiscount = 	(merchInfo->fSysPrice - merchInfo->fActualPrice) * merchInfo->fSaleQuantity/*nSaleCount*/;
				else 
					merchInfo->fBackDiscount = 	(merchInfo->fSysPrice - fActualPrice) * merchInfo->fSaleQuantity/*nSaleCount*/;
				//增加IsDiscountLabel, BackDiscount字段
				CString str_fSaleAmt_BeforePromotion;str_fSaleAmt_BeforePromotion.Format("%.2f",merchInfo->fSaleAmt_BeforePromotion);//aori add 2012-2-10 10:18:18 proj 2.79 测试反馈：条件促销时 应付错误：未能保存、恢复商品的促销前saleAmt信息。db增加字段保存该信息
//<--#服务调用接口#				
//				strSQL.Format("INSERT INTO SaleItem%s(SaleDT,ItemID,SaleID,"
//					"PLU,RetailPrice,SaleCount,SaleQuantity,SaleAmt,"
//					"PromoStyle,MakeOrgNO,PromoID,ManualDiscount, SalesHumanID, MemberDiscount,NormalPrice,ScanPLU,StampCount"//aori add StampCount 2011-8-30 10:13:21 proj2011-8-30-1 印花需求
//					",AddCol1 "//aori add 2012-2-10 10:18:18 proj 2.79 测试反馈：条件促销时 应付错误：未能保存、恢复商品的促销前saleAmt信息。db增加字段保存该信息
//					") VALUES ('%s',%d,%ld,'%s',%.4f,%ld,%.4f,%.4f,%d,%d,%d,%.4f,%d,%.4f,%.2f,'%s',%d"//aori add 2011-8-30 10:13:21 proj2011-8-30-1 印花需求
//					",'%s'"//aori add 2012-2-10 10:18:18 proj 2.79 测试反馈：条件促销时 应付错误：未能保存、恢复商品的促销前saleAmt信息。db增加字段保存该信息
//					")",
//					szTableDTExt, strSaleDT, nItemID, nSaleID, 
//					merchInfo->szPLU, fActualPrice, merchInfo->nSaleCount, 
//					fSaleQuantity, merchInfo->fSaleAmt,
//					merchInfo->nPromoStyle, 
//					merchInfo->nMakeOrgNO, merchInfo->nPromoID,
//					merchInfo->fItemDiscount,merchInfo->nSimplePromoID,merchInfo->fMemberShipPoint,merchInfo->fSysPrice,merchInfo->szPLUInput,merchInfo->StampCount////aori add 2011-8-30 10:13:21 proj2011-8-30-1 印花需求
//					,str_fSaleAmt_BeforePromotion //aori add 2012-2-10 10:18:18 proj 2.79 测试反馈：条件促销时 应付错误：未能保存、恢复商品的促销前saleAmt信息。db增加字段保存该信息
//					);
				if(m_ServiceImpl.HasSVMerch() )
					strSQL.Format("INSERT INTO SaleItem%s(SaleDT,ItemID,SaleID,"
						"PLU,RetailPrice,SaleCount,SaleQuantity,SaleAmt,"
						"PromoStyle,MakeOrgNO,PromoID,ManualDiscount, SalesHumanID, MemberDiscount,NormalPrice,ScanPLU,StampCount" 
						",AddCol1,AddCol2,AddCol3, AddCol4,ItemType,OrderID,CondPromDiscount,ArtID,OrderItemID,MemDiscAfterProm" 
						") VALUES ('%s',%d,%ld,'%s',%.4f,%ld,%.4f,%.4f,%d,%d,%d,%.4f,%d,%.4f,%.2f,'%s',%d" //%ld to %.4f 2013-2-27 15:30:11 proj2013-1-23-1 称重商品 加入 会员促销、限购
						",'%s','%d','%s','%d', %d,'%s',%.4f,'%s','%s',%.4f"
						")",
						szTableDTExt, strSaleDT, nItemID, nSaleID, 
						merchInfo->szPLU, fActualPrice, merchInfo->nSaleCount, 
						fSaleQuantity, merchInfo->fSaleAmt,
						merchInfo->nPromoStyle, 
						merchInfo->nMakeOrgNO, merchInfo->nPromoID,
						merchInfo->fItemDiscount,merchInfo->nSimplePromoID,merchInfo->fMemberShipPoint,merchInfo->fSysPrice,merchInfo->szPLUInput,merchInfo->StampCount 
						,str_fSaleAmt_BeforePromotion,m_ServiceImpl.GetServiceInfo()->nSVID,m_ServiceImpl.GetServiceInfo()->szSVNum,merchInfo->scantime,
						merchInfo->nItemType, merchInfo->strOrderNo,merchInfo->dCondPromDiscount,
 						merchInfo->strArtID,merchInfo->strOrderItemID,merchInfo->dMemDiscAfterProm);
				else
				{
					//若打印发票，将商品的发票信息保存到月表中[dandan.wu 2016-10-14]
					double dbInvoiceAmt = 0.0;

					if ( GetSalePos()->m_params.m_bAllowInvoicePrint )
					{
						//发票金额，若为订单商品，为0 [dandan.wu 2016-11-10]		
						if ( !merchInfo->strOrderNo.IsEmpty() )
						{
							dbInvoiceAmt = 0.0;
						}
						else
						{
							dbInvoiceAmt = merchInfo->fSaleAmt;
						}
					}

					strSQL.Format("INSERT INTO SaleItem%s(SaleDT,ItemID,SaleID,"
						"PLU,RetailPrice,SaleCount,SaleQuantity,SaleAmt,"
						"PromoStyle,MakeOrgNO,PromoID,ManualDiscount, SalesHumanID, MemberDiscount,NormalPrice,ScanPLU,StampCount" 
						",AddCol1, AddCol4,AddCol2,ItemType,OrderID,CondPromDiscount,ArtID,OrderItemID,MemDiscAfterProm"
						",InvoiceEndNo,InvoiceCode,InvoicePageCount,InvoiceAmt"
						") VALUES ('%s',%d,%ld,'%s',%.4f,%ld,%.4f,%.4f,%d,%d,%d,%.4f,%d,%.4f,%.2f,'%s',%d" //%ld to %.4f2013-2-27 15:30:11 proj2013-1-23-1 称重商品 加入 会员促销、限购
						",'%s','%d','%d',%d,'%s',%.4f,'%s','%s',%.4f" 
						",'%s','%s',%d,%.2f"
						")",
						szTableDTExt, strSaleDT, nItemID, nSaleID, 
						merchInfo->szPLU, fActualPrice, merchInfo->nSaleCount, 
						fSaleQuantity, merchInfo->fSaleAmt,
						merchInfo->nPromoStyle, 
						merchInfo->nMakeOrgNO, merchInfo->nPromoID,
						merchInfo->fItemDiscount,merchInfo->nSimplePromoID,merchInfo->fMemberShipPoint,merchInfo->fSysPrice,merchInfo->szPLUInput,merchInfo->StampCount 
						,str_fSaleAmt_BeforePromotion ,merchInfo->scantime ,merchInfo->IncludeSKU, merchInfo->nItemType, merchInfo->strOrderNo,merchInfo->dCondPromDiscount,
						merchInfo->strArtID,merchInfo->strOrderItemID,merchInfo->dMemDiscAfterProm
						,merchInfo->strInvoiceEndNo,m_strInvoiceCode,m_nInvoiceTotalPage,dbInvoiceAmt);		
				}
				int retn=pDB->Execute2(strSQL);CUniTrace::Trace(strSQL);
				if ( retn== 0 ){//aori change 2011-3-22 16:51:36 优化clientdb ：删除execute2
					unlock_saleitem_temp();//aori add 2011-8-7 8:38:11	proj2011-8-3-1 A 用 mutex等判断两个checkout之间 是否出现了update事件
					return false;
				}
			}
			//如果有操作日志，发送命令给
			if ( bHasCanceledItems ) {
				GetSalePos()->m_Task.putOperateRecord(m_stEndSaleDT, true);
			}
		}
		//CUniTrace::Trace("!!!最终保存后 各表内容---------此日志 到 1促销存储过程 日志之间 不应该出现对 saleitem_temp表进行更改的日志");//aori add 2011-8-3 17:03:07 proj2011-8-3-1 销售不平 条件促销 临时表 记日志里 //GetSalePos()->GetPromotionInterface()->WritePromotionLog( nSaleID);//aori add 2011-8-3 17:03:07 proj2011-8-3-1 销售不平 条件促销 临时表 记日志里 

		
		strSQL.Format("DELETE FROM Sale_Temp WHERE HumanID = %d;", nUserID);//清除临时数据
		pDB->Execute2(strSQL);//aori 2011-8-7 8:38:11  proj2011-8-3-1 用 mutex等判断两个cchange 2011-3-22 16:51:36 优化clientdb ：删除execute2
		strSQL.Format("DELETE FROM SaleItem_Temp WHERE HumanID = %d", nUserID);
		pDB->Execute2(strSQL);//aori 2011-8-7 8:38:11  proj2011-8-3-1 用 mutex等判断两个cchange 2011-3-22 16:51:36 优化clientdb ：删除execute2
		
		//EmptyTempSale();
		unlock_saleitem_temp();//aori add 2011-8-7 8:38:11	proj2011-8-3-1 A 用 mutex等判断两个checkout之间 是否出现了update事件
	} catch ( CException *pe ) {
		GetSalePos()->GetWriteLog()->WriteLogError(*pe);
		pe->Delete( );
		unlock_saleitem_temp();//aori add 2011-8-7 8:38:11	proj2011-8-3-1 A 用 mutex等判断两个checkout之间 是否出现了update事件
		return false;
	}
	return true;
}

//
//解挂某笔销售到当前销售中
//
//##ModelId=3E641FAB02FE
bool CSaleImpl::UnHangupSale(int nSaleID, SYSTEMTIME* stTime)
{
	SYSTEMTIME stNow;(stTime == NULL)?GetLocalTimeWithSmallTime(&stNow):stNow = *stTime;
	Clear();
	
	SYSTEMTIME stUnHunupDT;GetLocalTimeWithSmallTime(&stUnHunupDT);//aori move from down 2011-6-10 11:09:25 proj2011-6-10-1:生鲜挂起db错误
	if(!IsInSameDay(stUnHunupDT, m_stSaleDT)){//解决24小时问题，如果跨天需要重新获得时间
		m_nSaleID = m_pADB->GetNextSaleID(&stUnHunupDT);
	}
	m_stSaleDT = stUnHunupDT;
	m_nSaleID=nSaleID;//aori add 2011-6-10 11:09:25 proj2011-6-10-1:生鲜挂起db错误

	//((CPosApp *)AfxGetApp())->GetSaleDlg()->InitNextSale(SaleFinish_UnHangupSale,nSaleID);//aori add 2011-6-10 11:09:25 proj2011-6-10-1:生鲜挂起db错误
	//InitialNextSale(SaleFinish_UnHangupSale,nSaleID);//aori change 2011-6-10 11:09:25 proj2011-6-10-1:生鲜挂起db错误
	

	if ( !ReloadSale(nSaleID, stNow,true) ) 
		return false;
	WriteSaleLog(1);
	CString strSpanBegin, strSpanEnd, strSql;
	const char *szTableDTExt = GetSalePos()->GetTableDTExt(&stNow);

	strSpanBegin = GetDaySpanString(&stNow);
	strSpanEnd = GetDaySpanString(&stNow, false);
		//删除销售主表信息
		strSql.Format("DELETE FROM Sale%s WHERE SaleID = %lu "
			" AND SaleDT >= '%s' AND SaleDT < '%s' ",
			szTableDTExt, nSaleID, strSpanBegin, strSpanEnd);
	CAutoDB db(true);db.Execute(strSql);//aori change 2011-3-22 16:51:36 优化clientdb ：删除execute2

		//删除销售明细信息
		strSql.Format("DELETE FROM SaleItem%s WHERE SaleID = %lu" 
			" AND SaleDT >= '%s' AND SaleDT < '%s' ",
			szTableDTExt, nSaleID, strSpanBegin, strSpanEnd);
		db.Execute(strSql);//aori change 2011-3-22 16:51:36 优化clientdb ：删除execute2

		strSql.Format("%d", nSaleID);
		m_pADB->SetOperateRecord(OPT_UNHANGUP, GetSalePos()->GetCasher()->GetUserID(), strSql);
		GetSalePos()->m_Task.putOperateRecord(stNow);

	//将当前记录记录到临时表中
	SaveItemsToTemp();
	CString strSQL;strSQL.Format("INSERT INTO Sale_Temp(SaleDT,HumanID,MemberCode,Shift_No) VALUES"
		"('%s',%d,%s,%d)", GetFormatDTString(m_stSaleDT), 
		GetSalePos()->GetCasher()->GetUserID(),GetFieldStringOrNULL(m_MemberImpl.m_Member.szMemberCode),m_uShiftNo);
	db.Execute(strSQL);//aori change 2011-3-22 16:51:36 优化clientdb ：删除execute2

	return true;
}

//整单打折，参数为折让金额，有可能折让金额为负数（负折让）
//整单折扣率
//##ModelId=3E641FAC0061
void CSaleImpl::WholeBillRebate(double fRebateAmt, double fMemRebateAmt)
{	
	double fRest = 0.0, fMemRest = 0.0;
	
	//首先做会员的整单打折
	if(fMemRebateAmt != 0.0 )
	{
		double fMemAmt = GetTotalAmt(true);
		//折扣率比如0.98	
		double fMemRebateRate = 1.0 - (fMemRebateAmt / fMemAmt); 	
		for ( vector<SaleMerchInfo>::iterator merchInfo = m_vecSaleMerch.begin(); merchInfo != m_vecSaleMerch.end(); ++merchInfo ) {
			if ( (merchInfo->nMerchState == RS_NORMAL ) && ( merchInfo->fSaleQuantity/*nSaleCount*/ != 0 ) && (!merchInfo->bIsDiscountLabel)) {			
				fMemAmt = merchInfo->fSaleAmt;
				merchInfo->fSaleAmt *= fMemRebateRate;
				fMemRest += ReducePrecision(merchInfo->fSaleAmt);
				merchInfo->fMemSaleDiscount += fMemAmt - merchInfo->fSaleAmt;
				
			}
		}
	}
	//之后做手动整单打折之前做
	if(fRebateAmt != 0.0 )
	{
		double fAmt = GetTotalAmt();
		double fRebateRate = 1.0 - (fRebateAmt / fAmt); 
		for ( vector<SaleMerchInfo>::iterator merchInfo = m_vecSaleMerch.begin(); merchInfo != m_vecSaleMerch.end(); ++merchInfo ) {
			if ( (merchInfo->nMerchState != RS_CANCEL) && ( merchInfo->fSaleQuantity/*nSaleCount*/ != 0 ) && (!merchInfo->bIsDiscountLabel)) {			
				fAmt = merchInfo->fSaleAmt;
				merchInfo->fSaleAmt *= fRebateRate;
				fRest += ReducePrecision(merchInfo->fSaleAmt);merchInfo->fSaleAmt_BeforePromotion=merchInfo->fSaleAmt;//aori add 2012-2-7 16:11:04 proj 2.79 测试反馈：条件促销时 不找零 加 fSaleAmt_BeforePromotion
				merchInfo->fSaleDiscount += fAmt - merchInfo->fSaleAmt;
			}
		}
	}

	// fRest是各项销售的销售金额抹成小数点后两位后的余额累加
	// 将该项摊到价格最大并且销售额大于fRest的销售项上去
	//注意，会员的和非会员的要分开
	EraseMemOddment(fMemRest);
	EraseOddment(fRest);
}
//
//会员销售额抹零  
//
void CSaleImpl::EraseMemOddment(double fMemReducePrice)
{
	int nSID = 0;
	double fMaxPrice = 0.0f;
	vector<SaleMerchInfo>::iterator merchInfo = NULL;

	// 找出销售额大于抹零额并且价格最大的商品销售项进行抹零
	for ( merchInfo = m_vecSaleMerch.begin(); merchInfo != m_vecSaleMerch.end(); ++merchInfo ) {
		if( merchInfo->fSaleAmt > fMemReducePrice && merchInfo->fSaleQuantity/*nSaleCount*/ != 0 && merchInfo->fMemSaleDiscount > 0) {
			if ( merchInfo->fActualPrice > fMaxPrice ) {
				fMaxPrice = merchInfo->fActualPrice;
				nSID = merchInfo-m_vecSaleMerch.begin();
			}
		}
	}

//	如果不存在，则取第一项(即nSID=0，一直没有变化

	// 修改商品的销售额，折扣额以及实际售价
	//fMaxPrice will be a temp double var here
	merchInfo = &m_vecSaleMerch[nSID];
	fMaxPrice = merchInfo->fSaleAmt;
	merchInfo->fSaleAmt += fMemReducePrice;
	merchInfo->fMemSaleDiscount += ReducePrecision(merchInfo->fSaleAmt);merchInfo->fSaleAmt_BeforePromotion=merchInfo->fSaleAmt;//aori add 2012-2-7 16:11:04 proj 2.79 测试反馈：条件促销时 不找零 加 fSaleAmt_BeforePromotion
	merchInfo->fMemSaleDiscount += fMaxPrice - merchInfo->fSaleAmt;
}

//
//销售额抹零  
//我改！！！改成找出销售额大于抹零额并且价格最大的商品销售项进行抹零
//
//##ModelId=3E641FAB02C2
void CSaleImpl::EraseOddment(double fReducePrice)
{
	int nSID = 0;
	double fMaxPrice = 0.0f;
	vector<SaleMerchInfo>::iterator merchInfo = NULL;

	// 找出销售额大于抹零额并且价格最大的商品销售项进行抹零
	for ( merchInfo = m_vecSaleMerch.begin(); merchInfo != m_vecSaleMerch.end(); ++merchInfo ) {
		if( merchInfo->fSaleAmt > fReducePrice && merchInfo->fSaleQuantity/*nSaleCount*/ != 0 ) {
			if ( merchInfo->fActualPrice > fMaxPrice ) {
				fMaxPrice = merchInfo->fActualPrice;
				nSID = merchInfo-m_vecSaleMerch.begin();
			}
		}
	}

//	如果不存在，则取第一项(即nSID=0，一直没有变化

	// 修改商品的销售额，折扣额以及实际售价
	//fMaxPrice will be a temp double var here
	merchInfo = &m_vecSaleMerch[nSID];
	fMaxPrice = merchInfo->fSaleAmt;
	merchInfo->fSaleAmt += fReducePrice;
	merchInfo->fSaleDiscount += ReducePrecision(merchInfo->fSaleAmt);merchInfo->fSaleAmt_BeforePromotion=merchInfo->fSaleAmt;//aori add 2012-2-7 16:11:04 proj 2.79 测试反馈：条件促销时 不找零 加 fSaleAmt_BeforePromotion
	merchInfo->fSaleDiscount += fMaxPrice - merchInfo->fSaleAmt;
}
// bool CSaleImpl::checkDiscountLabelBar(){//aori add 2012-2-14 14:34:07 proj 2012-2-13 折扣标签校验
// 	
// 	int nCount = -1;ETICommand ecmd;//aori add bug 2011-1-12 8:43:33 内存泄露
// 	ETICommand* m_pETICmd=&ecmd;//aori add 2010-12-27 21:08:01 bug 
// 	memset(m_pETICmd->szParam, 0, ETI_CMD_CONTENT_SIZE);
// 	memcpy((void *)&m_pETICmd->szParam, (const void *)&stLastUpdate, sizeof(SYSTEMTIME));
// 	m_pETICmd->nCmdID = ETI_DOWNLOAD_getSale_V20111108;
// 	m_pETICmd->nParam = nSaleID;
// 	m_pETICmd->nAuxID = nPosID;
// 	m_pETICmd->nSize = sizeof(SYSTEMTIME);
// 	
// 	if ( m_pFTS->Send(m_pETICmd) == SOCKET_ERROR ) {
// 		return SOCKET_ERROR;
// 	}
// 	if ( m_pFTS->Receive(m_pETICmd) == SOCKET_ERROR ) {
// 		return SOCKET_ERROR;
// 	}
// }

//
//	新增商品明细
//
//======================================================================待
// 函 数 名：AddItem
// 功能描述：新增商品明细
// 输入参数：
// 输出参数：bool
// 创建日期：00-2-21
// 修改日期：2009-4-2 曾海林
// 作  者：***
// 附加说明：促销变更修改,需要重新整理
//======================================================================/

int CSaleImpl::AddItem(CString szPLU, Inputer inputer,bool bStamp,int scantime,
					   CString strOrderNo, CString strOrderOtemID, double dCondPromDis,
					   double dSaleQty, double dSaleAmt, int nDiscType, double dOrderSalePrice)
{//首先判断该商品是否已经存在，已经存在的话，大部分信息不必要从数据库中读取了
//这样可以提高销售速度,但是金额型的还是重新取，不然条码称打的条码如果一样m_bCanChangePrice标志就不对了
	SaleMerchInfo merchInfo("");
	SaleMerchInfo* existMerchInfo = inputer == Inputer_UpLimit_simulate ? NULL : GetSaleMerchInfo(szPLU, bStamp ? DS_YH : DS_NO);
	// bool bPriceInBarcode=false;//写入13位码日志 //aori del
// 	if ( existMerchInfo && existMerchInfo->nManagementStyle == MS_DP) {//aori change bug:
// 		merchInfo=*existMerchInfo;//2013-2-4 17:15:49 proj2013-1-17-3 优化 saleitem 输入相同商品崩溃memcpy(&merchInfo, (void *)existMerchInfo, sizeof(SaleMerchInfo));
// 		merchInfo.bLimitSale=FALSE;//aori add for bug:nonmember get memberprice
// 		merchInfo.bIsDiscountLabel=FALSE;//aori add 2011-1-4 9:41:08 区分同一商品的折扣和限购
// 		merchInfo.nDiscountStyle &= 0xff;
// 		//如果商品是折扣促销，则判断是否已经满足下限在打折了
// 		if ( !bStamp&&!((merchInfo.nDiscountStyle == DS_ZK || merchInfo.nDiscountStyle == DS_TZ)&& merchInfo.nMerchState == RS_DS_ZK )) {
// 			merchInfo.fActualPrice = merchInfo.fLabelPrice;
// 			merchInfo.nMerchState = RS_NORMAL;
// 		}
// 		merchInfo.nSaleCount = 0;
// 		//m_bCanChangePrice = (merchInfo.nManagementStyle != MS_DP);2013-1-21 11:25:41 proj2013-1-17优化 saleitem ：去掉saleimpl商品属性//aori 金额类可以变价 2011-9-14 9:51:39 优化 将程序中所有m_bCanChangePrice的判断改为对当前商品的判断
// 	} else 
	{
		merchInfo=SaleMerchInfo(szPLU,GetMember().szMemberCode,scantime,strOrderNo, dCondPromDis, 
								dSaleQty, strOrderOtemID, dSaleAmt, nDiscType, dOrderSalePrice);//CSaleItem saleItem(szPLU,GetMemberCode());
		
		if ( !merchInfo.m_saleItem.IsValidMerch() ) 
			return -2;
		
		if ( !merchInfo.m_saleItem.IsAllowSale() ) 
			return -3;
		//saleItem.FormatSaleMerchInfo(merchInfo);aori del 2013-1-21 11:52:27 proj2013-1-17 16:32:11 优化 saleitem

		m_bCanChangePrice = merchInfo.m_saleItem.CanChangePrice();	//设置是否可以更改价格
		m_bFixedPriceAndQuantity = merchInfo.m_saleItem.IsFixedPriceAndQuantity();//取促销价（如果有的话）//m_promotion.GetPromotionPrice(....hInfo, GetMemberLevel(), bStamp);

		if( !bHasConfirmedMemberSale && !IsMember() && merchInfo.fRetailPrice2 < merchInfo.fRetailPrice
			&& !merchInfo.bFixedPriceAndQuantity && !merchInfo.bIsDiscountLabel)
		{
			CConfirmLimitSaleDlg confirmDlg;
			confirmDlg.m_strConfirm.Format("此为会员商品，是否按原价进行购买");
			if ( confirmDlg.DoModal() == IDCANCEL )
				return -1;
			bHasConfirmedMemberSale = true;
		}//end aori add 2010-12-25 16:01:49 非会员购买会员商品时 询问是否按原价购买（或输入会员号购买）。 later//strcpy(merchInfo.szPLUInput, szPLU);//aori del optimize 2011-9-19 15:45:13 merchInfo.szPLUInput的生成
		
		//普通商品，销售码即为输入码/扫码；订单商品需要根据店内码获得主条码 [dandan.wu 2016-5-13]
		//merchInfo.szPLUInput=szPLU;
		if ( !szPLU.IsEmpty() )
		{
			if ( strOrderNo.IsEmpty() )
			{
				merchInfo.szPLUInput = szPLU;
			}
			else
			{
				CString strBarcode(_T(""));
				CAccessDataBase* pADB = &GetSalePos()->m_pADB;
				pADB->GetBarCodeByMerchID(szPLU,strBarcode);
				if ( !strBarcode.IsEmpty() )
				{
					merchInfo.szPLUInput = strBarcode;
				}
				else
				{
					merchInfo.szPLUInput=szPLU;
				}	
			}
		}
		
		// 自营称重商品不允许输入六位码
		if (merchInfo.nManagementStyle==MS_JE && merchInfo.nMerchType== 2 && strlen(merchInfo.szPLUInput) == GetSalePos()->GetParams()->m_nMerchCodeLen)
			return -4;//zenghl 20090619
		
		// 联营称重商品输入方式： 参数：PoolScaleMerchSaleMode
		if (merchInfo.nMerchType==5 && strlen(merchInfo.szPLUInput)==GetSalePos()->GetParams()->m_nMerchCodeLen)
		{
			CString msg;
			switch(GetSalePos()->GetParams()->m_nPoolScaleMerchSaleMode)
			{
			case 2:	//录入SKU时提示,允许录入
				msg.Format("该商品为磅秤商品，不建议直接输入商品码销售! \n请扫描磅秤标签，或提报主数据取消磅秤标识!");
				CUniTrace::Trace(msg);
				if ( IDOK !=  PosMessageBox(msg,MB_OKCANCEL))
					return -4;
				break;
			case 1:	//不允许录入SKU
				return -4;
			case 0:  //不控制
				break;
			}
		}
		if ( bStamp && merchInfo.nDiscountStyle != DS_YH ) 
		{//如果收银员指定本销售项是印花销售，但是在数据库却找不到生效可用的印花促销//则提示收银员，如果确定继续购买，则不走印花销售，如果满足折扣促销的话，//执行折扣促销价，否则取零售价
			if ( IDOK != MessageBox(((CPosApp *)AfxGetApp())->GetSaleDlg()->GetSafeHwnd(), "该商品目前没有生效印花促销，是否继续购买？", "销售", MB_OKCANCEL | MB_ICONWARNING ) ) //aori 2012-2-29 14:53:17 测试反馈  proj 2012-2-13 折扣标签校验 ：提示框应死循环			
				return -1;
		}
		IsDiscountLabelBar = merchInfo.m_saleItem.m_bIsDiscountLabelBar;//是否折扣//bPriceInBarcode=saleItem.m_bPriceInBarcode;//aori del no use//aori add 判断该商品是否按限购处理:函数功能:判断是否按限购处理该商品，如是则merchInfo.bLimitSale置为真
		if(IsDiscountLabelBar&&!GetSalePos()->GetPOSClientImpl()->checkDiscountLabel(szPLU,true)){//aori add 2012-2-16 14:45:33 proj 2012-2-13 折扣标签校验 加消息 ETI_DOWNLOAD_CHECK_DiscountLabel
			return -1;
		}
	}
	
	//if(merchInfo.nManagementStyle==2 && ( merchInfo.nMerchType==5||merchInfo.nMerchType==6)  &&  strlen(merchInfo.szPLUInput)==13 )//处理联营金额类商品merchtype=5 and ManagementStyle=2	
	//若为订单，直接取数量；若为现货，数量初始化为1  [dandan.wu 2016-4-22]
	if ( strOrderNo.IsEmpty() )
	{
		merchInfo.fSaleQuantity = 0;
	}
	else
	{
		merchInfo.fSaleQuantity = dSaleQty;
	}
	
	if (merchInfo.bFixedPriceAndQuantity)
	{//||merchInfo.bPriceInBarcode//aori del 2011-9-22 10:30:33 ){//aori add SteelYardPriceControl proj2011-8-15-2 称重实际价离retailprice偏差较大时 记日志。
		bool bOutCircul=false;
		double fanwei,biaozhun=(IsMember()?min(merchInfo.fRetailPrice2,merchInfo.fSysPrice):merchInfo.fSysPrice),actual_Upon_Retail=merchInfo.fActualPrice-biaozhun;
		if (actual_Upon_Retail==0)
			fanwei=0;
		else if(actual_Upon_Retail>0)
			fanwei=actual_Upon_Retail-biaozhun*GetSalePos()->GetParams()->m_SteelYardPriceUpLimit/100;
			 else	
				 fanwei=-actual_Upon_Retail-biaozhun*GetSalePos()->GetParams()->m_SteelYardPriceDownLimit/100;
		if (fanwei>0&&merchInfo.ChgSteelyardPrice==0)	
		{//aori add ChgSteelyardPrice proj2011-9-6-1 对磅秤允许变价的商品, 在POS端不进行磅秤价格报警  "1" -磅秤可变价   "0" -磅秤不可变价null 默认为"0"
			CString hehe, msg;
			hehe.Format("13/18码商品%s 实价%.3f超限%.3f,原价%.3f,上限%d,下限%d,处理方式：%d",merchInfo.szPLU,merchInfo.fActualPrice,actual_Upon_Retail,biaozhun,GetSalePos()->GetParams()->m_SteelYardPriceUpLimit,GetSalePos()->GetParams()->m_SteelYardPriceDownLimit,GetSalePos()->GetParams()->m_SteelYardPriceControl);
			CUniTrace::Trace(hehe);
			switch (GetSalePos()->GetParams()->m_SteelYardPriceControl)
			{//0-不处理;1-警告(允许销售);2.报错(不允许销售)
			case 0:
				break;
			case 1:msg.Format( "该商品销售价%.3f超出原价格(%.3f)的范围%s限(百分之%d)，是否继续销售？",merchInfo.fActualPrice,biaozhun,actual_Upon_Retail>0?"上":"下",actual_Upon_Retail>0?GetSalePos()->GetParams()->m_SteelYardPriceUpLimit:GetSalePos()->GetParams()->m_SteelYardPriceDownLimit);
				if ( IDOK !=  PosMessageBox(msg,MB_OKCANCEL))//(((CPosApp *)AfxGetApp())->GetSaleDlg()->GetSafeHwnd(),msg, "销售", MB_OKCANCEL| MB_ICONWARNING|MB_DEFBUTTON2 ) ) //aori change //aori 2012-2-29 14:53:17 测试反馈  proj 2012-2-13 折扣标签校验 ：提示框应死循环			
					return -1;
				break;//aori 2012-2-29 14:53:17 测试反馈  proj 2012-2-13 折扣标签校验 ：提示框应死循环			
			case 2:msg.Format( "该商品销售价%.3f超出原价格(%.3f)的范围%s限(百分之%d)，不允许销售\n\n  请按【取消】键返回",merchInfo.fActualPrice,biaozhun,actual_Upon_Retail>0?"上":"下",actual_Upon_Retail>0?GetSalePos()->GetParams()->m_SteelYardPriceUpLimit:GetSalePos()->GetParams()->m_SteelYardPriceDownLimit);
				//PosMessageBox(msg);//MessageBox(((CPosApp *)AfxGetApp())->GetSaleDlg()->GetSafeHwnd(),msg, "销售", MB_OK| MB_ICONWARNING ) ;//aori 2012-2-29 14:53:17 测试反馈  proj 2012-2-13 折扣标签校验 ：提示框应死循环			
				while ( IDCANCEL != MessageBox(NULL,msg, "商品销售", MB_OKCANCEL | MB_ICONWARNING|MB_DEFBUTTON1 ));//aori change 2012-2-29 14:53:17 测试反馈	proj 2012-2-13 折扣标签校验 ：提示框应死循环			
				return -1;
				break;
			}
		}
	}
	int nSID = GetSupperAddSID(merchInfo.szPLU, merchInfo.fActualPrice, merchInfo.nDiscountStyle,merchInfo.m_saleItem.m_rawSaleMerch.IncludeSKU,
								merchInfo.strOrderNo, merchInfo.strOrderItemID) ;//如果该商品是可追加的商品，则直接返回该SID，否则新增一项
	if (inputer==Inputer_UpLimit_simulate)
		nSID=-1;
	if ( nSID  < 0 )
	{
		nSID=merchInfo.nSID = m_vecSaleMerch.size();
		if(!m_bFixedPriceAndQuantity && strOrderNo.IsEmpty())
			merchInfo.fSaleAmt_BeforePromotion=merchInfo.fSaleAmt = 0.0f;	//如果不能修改商品销售数量（18码）就不重新计算总金额//aori add merchInfo.fSaleAmt 
		if(SetItemSalesMan(merchInfo))
		{
			m_vecSaleMerch.push_back(merchInfo);//aori later 2011-9-6 9:49:29 proj2011-8-30-1.1 印花需求 加读取salemerc表的StampRate（add1）字段
		   	SaveItemToTemp(nSID);
		}
		else
		{
			AfxMessageBox("添加新merch时失败,联系总部");//aori 2012-2-29 14:53:17 测试反馈  proj 2012-2-13 折扣标签校验 ：提示框应死循环			
			nSID = -1;
		}
	} 
	else
	{
		UpdateItemInTemp_scantime(nSID,scantime);		
	}
	return nSID;
} 


//
// 	添加新的商品销售明细
//
//##ModelId=3E641FAC029B
int CSaleImpl::AddItem(SaleMerchInfo &newMerchInfo)
{
	newMerchInfo.nSID = m_vecSaleMerch.size();

	m_vecSaleMerch.push_back(newMerchInfo);
	//add lxg.03/3/26存入临时商品表
	SaveItemToTemp(newMerchInfo.nSID);

	return newMerchInfo.nSID;
}

//
//	修改销售数量(累加)
//
//##ModelId=3E641FAB0286
bool CSaleImpl::ModifyQuantity(int &CurSID, double/*int*/& dbInputCount, double& fSaleAmt,Inputer inputer)//aori change 因为nSaleCount可能会被更改
{
	//SaleMerchInfo *merchInfo = GetSaleMerchInfo(nSID);//aori change limit sale
	if(CurSID>=m_vecSaleMerch.size()||CurSID<0){
		AfxMessageBox("saleID wrong when modifyQuantity");//aori 2012-2-29 14:53:17 测试反馈  proj 2012-2-13 折扣标签校验 ：提示框应死循环			
		return false;
	}//aori add 2011-3-8 10:50:36 2.55.limit--
	vector<SaleMerchInfo>::iterator merchInfo = m_vecSaleMerch.begin()+CurSID;
	if ( merchInfo != NULL && merchInfo->nMerchState != RS_CANCEL)
	{
		m_stratege_xiangou.HandleLimitSaleV3( &merchInfo,dbInputCount,TRIGGER_TYPE_NoCheck,inputer);//2013-2-28 10:41:10 proj2013-1-23-1 称重商品 加入 会员促销、限购			

		//商品累加到merchInfo->fSaleQuantity,若为拆零商品，nSaleCount记录笔数；否则，nSaleCount也记录数量 [dandan.wu 2016-4-19]
		merchInfo->fSaleQuantity += dbInputCount;
		if ( merchInfo->nMerchType == OPEN_STOCK_2 || merchInfo->nMerchType == OPEN_STOCK_5 )
		{
			merchInfo->nSaleCount = 1;
		}
		else
		{
			merchInfo->nSaleCount += dbInputCount;//aori change move to handleLimitSale
		}
		
		if(merchInfo->bFixedPriceAndQuantity&&merchInfo->fSaleQuantity==0||
				(!merchInfo->bFixedPriceAndQuantity&&merchInfo->fSaleQuantity/*nSaleCount*/==0))
		{
			
			CString strSQL;
			strSQL.Format("DELETE FROM Saleitem_Temp WHERE HumanID = %d and  ItemId=%d and ScanPlu=%s;", 
																	GetSalePos()->GetCasher()->GetUserID(),
																	CurSID,
																	merchInfo->m_saleItem.m_szInput_PLU);	
			CUniTrace::Trace(strSQL);			
			CAutoDB db;db.Execute(strSQL);
			m_vecSaleMerch.pop_back();
			CurSID==0?1:CurSID--;
			return true;
		}//aori add end 2011-1-7 8:33:29 取消超限时产生0数量限购内商品
		
		ReCalculate(*merchInfo);
		fSaleAmt = merchInfo->fSaleAmt;
		
		//销售明细日志  zenghl 20090608
		SaleMerchInfo merchInfo_log=*merchInfo;
// 		if ( merchInfo_log.nMerchType == OPEN_STOCK_2 || merchInfo_log.nMerchType == OPEN_STOCK_5 )
// 		{
// 			merchInfo_log.fSaleQuantity += dbInputCount;
// 			merchInfo_log.nSaleCount = 1;
// 		}
// 		else
// 		{
// 			merchInfo_log.nSaleCount += dbInputCount;//aori change move to handleLimitSale
// 		}

		WriteSaleItemLog(merchInfo_log,0,1);
		return true;
	} 
	else 
		return false;
}

//
//根据价格和数量重算某项销售项的SaleAmt和SaleDiscount, SaleQuantity
//
//##ModelId=3E641FAC02D7
void CSaleImpl::ReCalculate(SaleMerchInfo &merchInfo)
{
	if(merchInfo.fManualDiscount != 0)//MarkDown/MarkUp后的不重新算
		return;

	//重算金额和折扣,如果不能修改商品销售数量（18码）就不重新计算总金额
	if(merchInfo.bIsDiscountLabel)
	{
		merchInfo.fSaleAmt = merchInfo.fActualPrice * merchInfo.fSaleQuantity/*nSaleCount*/;
		merchInfo.fSaleDiscount *=merchInfo.fSaleQuantity/*nSaleCount*/;
	}
	else
	{
		if(merchInfo.IncludeSKU>1)
		{
			merchInfo.fSaleAmt = merchInfo.fActualBoxPrice * merchInfo.fSaleQuantity/merchInfo.IncludeSKU - merchInfo.fSaleDiscount;
		}
		else
		{
			merchInfo.fSaleAmt = merchInfo.fActualPrice*merchInfo.fSaleQuantity - merchInfo.fSaleDiscount;
		}
	}

	//精度为小数点后两位 ZouXuehai Add 20160620
	{
		ReducePrecision(merchInfo.fSaleAmt);
	}

	merchInfo.fSaleDiscount += ReducePrecision(merchInfo.fSaleAmt);
	merchInfo.fSaleAmt_BeforePromotion = merchInfo.fSaleAmt; 
	if(merchInfo.bMemberPromotion && merchInfo.nMerchState != RS_NORMAL && merchInfo.nMerchState != RS_CANCEL && !merchInfo.bIsDiscountLabel)
	{//重新计算会员促销的折扣
		//merchInfo.fMemSaleDiscount = (merchInfo.fLabelPrice - merchInfo.fZKPrice) * (merchInfo.bFixedPriceAndQuantity ? merchInfo.fSaleQuantity : merchInfo.nSaleCount);
		//zenghl@nes.com.cn 20090429
	}
}

//
//	修改商品售价
//
//##ModelId=3E641FAB0254
double CSaleImpl::ModifyPrice(int nSID, double fActualPrice)//aori 2011-9-2 17:29:03 产品价格数量生成 优化
{
	SaleMerchInfo *merchInfo = GetSaleMerchInfo(nSID);
	if ( merchInfo == NULL )
		return 0.0f;

	//只有非单品类（金额、联销）才能会用到修改价格
	//修改的价格就是实售价，系统价格是数据库的价格，总是不会变的
	//单品的标签价与系统价一致
	//非单品类的标签价为收银员录入的价格
	merchInfo->fActualPrice = fActualPrice;
	if ( merchInfo->nManagementStyle != MS_DP ) 
	{
		merchInfo->fLabelPrice = fActualPrice;
	}
	WriteSaleItemLog(*merchInfo,0,2);//aori add 2011-5-13 10:53:09 proj2011-5-11-2：销售不平5-9北科大3: 日志级别 writelog写成dll// 记日志 zenghl 20090608

	ReCalculate(*merchInfo);
	
	return merchInfo->fSaleAmt;
}

//	根据SID取销售明细
//##ModelId=3E641FAB022C
SaleMerchInfo * CSaleImpl::GetSaleMerchInfo(int nSID)
{
	if(nSID>=m_vecSaleMerch.size()||nSID<0)
		return NULL;//aori add 2012-3-15 11:02:24 proj 2012-3-5 消息机制bug 潜在bug
	return &m_vecSaleMerch[nSID];
}

// 判断某项商品是否是可追加的，可则返回将要追加的nSID上
// 判断条件：存在、单品、折扣方式、价格一致，(不是折扣标签)、不是拆零商品且没有作废并且必须是连续输入的商品。返回该商品的SID
//##ModelId=3E641FAB01F0
//[in]:strOrderID : 订单号
//[in]:strOrderItemID:商品在订单中的ItemID（saleorderItem表中ItemID  只有订单商品才有）
int CSaleImpl::GetSupperAddSID(CString pPLU, double fActualPrice, short nDiscountStyle,int includesku,
							   CString strOrderID, CString strOrderItemID)
{

	if ( m_vecSaleMerch.size() > 0 ) 
	{
		char *szPLU = (LPSTR)(LPCTSTR)pPLU;
		SaleMerchInfo* merchInfo = &m_vecSaleMerch[m_vecSaleMerch.size() - 1];
		
		if ( merchInfo->nManagementStyle == MS_DP 
			&& strcmp(merchInfo->szPLU, szPLU) == 0 
			&& merchInfo->fActualPrice == fActualPrice
			&& merchInfo->nDiscountStyle == nDiscountStyle
			&& merchInfo->nMerchState != RS_CANCEL   
		 	&& !merchInfo->bIsDiscountLabel
			&& merchInfo->IncludeSKU==includesku
			&& merchInfo->strOrderNo == strOrderID
			&& merchInfo->strOrderItemID == strOrderItemID
			&& !(merchInfo->nMerchType == OPEN_STOCK_2 || merchInfo->nMerchType == OPEN_STOCK_5)) 
		{
			return m_vecSaleMerch.size() - 1;
		}
	}
/*	for ( vector<SaleMerchInfo>::iterator merchInfo = m_vecSaleMerch.begin(); merchInfo != m_vecSaleMerch.end(); ++merchInfo) {
		if ( merchInfo->nManagementStyle == MS_DP && strcmp(merchInfo->szPLU, szPLU) == 0 
			&& merchInfo->nDiscountStyle == nDiscountStyle
			&& merchInfo->nMerchState != RS_CANCEL ) {
			if ( merchInfo->nDiscountStyle == DS_ZK 
				&& merchInfo->nSaleCount > merchInfo->nLowLimit
				&& ( (merchInfo->nSaleCount < merchInfo->nUpLimit ) || merchInfo->nUpLimit == 0 )
				) {
				return merchInfo->nSID;
			} else if ( merchInfo->fActualPrice == fActualPrice ) {
				return merchInfo->nSID;
			}
		}
	}
*/	return -1;	
}

//手动打折某销售项，一旦手动打折，则该销售项不再参与其他任何促销（折扣、组合）
//##ModelId=3E641FAB01BE
bool CSaleImpl::SetItemDiscount(int nSID, double fDiscount, short nDiscountStyle, int nDiscountID)
{
	if ( nSID >= m_vecSaleMerch.size() ) 
		return false;

	vector<int> vecIndex;
	if(m_vecSaleMerch[nSID].nItemType == ITEM_IN_ORDER)
	{
		CString strOrderNO = m_vecSaleMerch[nSID].strOrderNo;
		for (int n = 0 ; n < (int)m_vecSaleMerch.size(); n++) 
		{
			if(m_vecSaleMerch[n].nItemType == ITEM_IN_ORDER && m_vecSaleMerch[n].strOrderNo == strOrderNO)
				vecIndex.push_back(n);	
		}	
	}
	else
		vecIndex.push_back(nSID);

	double dRate = fDiscount;
	if ( nDiscountStyle == SDS_PRICE) 
	{
		if(m_vecSaleMerch[nSID].nItemType == ITEM_IN_ORDER)
		{
			double dTotalAmt = 0;
			for(int n = 0; n < (int)vecIndex.size(); n++)
			{
				int nIndex = vecIndex[n];
				if(!m_vecSaleMerch[nIndex].bIsDiscountLabel)
					dTotalAmt += m_vecSaleMerch[nIndex].fSaleAmt;
			}
			if(dTotalAmt > 0)
			{
				dRate = fDiscount/dTotalAmt*100;
			}
		}
		else
		{
			dRate = fDiscount/m_vecSaleMerch[nSID].fSaleAmt*100;
		}
	}

	for(int m = 0; m < (int)vecIndex.size(); m++)
	{
		int nIndex = vecIndex[m];
		if(nIndex >= m_vecSaleMerch.size())
			return false;

		SaleMerchInfo *merchInfo = &m_vecSaleMerch[nIndex];
		if ( merchInfo->fSaleQuantity/*nSaleCount*/ == 0 )
			return false;
		if( merchInfo->bIsDiscountLabel)
			continue;
		//如果是手动打折，则利用这两个临时变量存放折扣率/折扣价，
		merchInfo->fZKPrice = dRate;
		merchInfo->nLowLimit = SDS_RATE;
		
		//merchInfo->fManualDiscount = merchInfo->fSaleAmt * (1-dRate/100);//markdown
		double fSaleAmt = (merchInfo->fSaleAmt * dRate)/100.0;
		ReducePrecision(fSaleAmt);

		merchInfo->fSaleDiscount += merchInfo->fSaleAmt - fSaleAmt;
		merchInfo->fManualDiscount = merchInfo->fSaleAmt - fSaleAmt;//fManualDiscount>0:markdown,fManualDiscount<0:MarkUp [dandan.wu 2017-11-1]

		//不用再四舍五入 [dandan.wu 2016-1-24]
//		ReducePrecision(merchInfo->fSaleDiscount);
//		ReducePrecision(merchInfo->fManualDiscount);
		merchInfo->fSaleAmt = fSaleAmt;
		merchInfo->fSaleAmt_BeforePromotion=merchInfo->fSaleAmt;//aori add 2012-2-7 16:11:04 proj 2.79 测试反馈：条件促销时 不找零 加 fSaleAmt_BeforePromotion

		//merchInfo->nDiscountStyle |= DS_MANUAL;
		merchInfo->nDiscountStyle &= DS_MANUAL; //应该改为按位与，0xff&任何一个数=该数，[modified by dandan.wu 2016-2-25]
		
		//delete by wjx
		// 	//前台输入折扣，商品实际价格改变 [dandan.wu 2016-2-25]
		// 	merchInfo->fActualPrice = (merchInfo->fActualPrice*fDiscount)/100.0;
		// 
		// 	//前台输入折扣，商品实际箱码价格改变 [dandan.wu 2016-2-25]
		// 	merchInfo->fActualBoxPrice = (merchInfo->fActualBoxPrice*fDiscount)/100.0;
		// 
		// 	//记录手工折扣 [dandan.wu 2016-2-25]
		// 	merchInfo->fManualDiscount = fDiscount;
		
		merchInfo->nMerchState = RS_DS_MANUAL;
		merchInfo->nDiscountID = nDiscountID;
	}

	return true;
}


//======================================================================
// 函 数 名：SetMemberCode
// 功能描述：设置本笔销售的会员信息
// 输入参数：
// 输出参数：bool
// 创建日期：00-2-21
// 修改日期：2009-4-1 曾海林
// 作  者：***
// 附加说明：数据库变更修改 Sale_Temp
//======================================================================
int CSaleImpl::SetMemberCode(const char *szMemberCode )//,double dTotConsume, double dTotIntegral, short nCardStyle)
{
	return m_MemberImpl.QueryMember(szMemberCode,VIPCARD_NOTYPE_FACENO)?1:0;
   /*
	if(m_MemberImpl.SetMember(szMemberCode, dTotConsume, dTotIntegral, nCardStyle))
	{
		if ( !m_MemberImpl.m_Member.bEnable //|| !IsValidDate(m_MemberImpl.m_Member.stEndDate) ) {
			m_MemberImpl.Clear();
			return -1;
		}
	} else { 
		m_MemberImpl.Clear();
		return 0;
	}

	//记录
	CPosClientDB* pDB = CDataBaseManager::getConnection(true);
	if ( !pDB ) {
		CUniTrace::Trace("CSaleImpl::SetMemberCode");
		CUniTrace::Trace(MSG_CONNECTION_NULL, IT_ERROR);
		return-3;//aori change 2011-2-23 10:19:34//aori change 2011-3-27 13:24:11 socket 优化	vss2.55.0.15 ok11 CSaleImpl::SetMemberCode 返回值要更改
	}
	string strCC;
	if ( m_strCustomerCharacter.size() > 0 ) {
		strCC = "CharCode = '";
		strCC += m_strCustomerCharacter;
		strCC += "'";
	} else
		strCC = "CharCode IS NULL";
	
	CString member,cardinput;
	int card_type;
	member.Format("%s",m_MemberImpl.m_Member.szMemberCode);
	int indexs=member.Find("=",0);
	if (indexs > 0)
	{
		member=member.Left(indexs);//会员号,去掉=后面的东西
	}
	if (GetSalePos()->GetPosMemberInterface()->Cardinno.GetLength() > 1)
	{
		cardinput = GetSalePos()->GetPosMemberInterface()->Cardinno;
		card_type = 0;
	}
	else
	{
		cardinput = member;
		card_type = 1;

	}
	member=member.Left(18);

	CString strSQL; //mabh
	strSQL.Format("UPDATE Sale_Temp SET MemberCode= '%s' ,  MemCardNO = '%s' , MemCardNOType = %d "  
		" WHERE SaleDT = '%s' AND %s AND HumanID = %d  ", 
		member,cardinput,card_type,//GetFieldStringOrNULL(m_MemberImpl.m_Member.szMemberCode),
		GetFormatDTString(m_stSaleDT), strCC.c_str(), GetSalePos()->GetCasher()->GetUserID());
		pDB->Execute2(strSQL);//aori change 2011-3-22 16:51:36 优化clientdb ：删除execute2
	
	CDataBaseManager::closeConnection(pDB);
	//写日志
	CString str_logBuf;
	str_logBuf.Format("会员卡刷卡成功,会员卡号:%s",m_MemberImpl.m_Member.szMemberCode);
	GetSalePos()->GetWriteLog()->WriteLog(str_logBuf,4);//[TIME OUT(Second) 0.000065]
	//get remove memberscore zenghl@nes.com.cn 20090428
	int iReturn=0,nUseRemoteMemScore=GetSalePos()->m_params.m_nUseRemoteMemScore;//是否获取远程积分
	if(strlen(m_MemberImpl.m_Member.szMemberCode)>1){//aori change 2011-3-28 16:29:52 CSaleImpl::SetMemberCode 
		switch(nUseRemoteMemScore){
			case 2:		
				iReturn=GetSalePos()->GetPosMemberInterface()->ReadMembershipPointFromST(m_MemberImpl.m_Member.szMemberCode);
				break;
			case 1: //获取会员远程实时积分 
				GetSalePos()->GetWriteLog()->WriteLog("开始读取会员积分1",3);//[TIME OUT(Second) 0.000065]
				iReturn=GetSalePos()->GetPosMemberInterface()->ReadMemberCardInfo(m_MemberImpl.m_Member.szMemberCode,nUseRemoteMemScore)?1:-1;
				break;
			case 3: //获取会员远程实时积分和st积分  mabh 
				GetSalePos()->GetWriteLog()->WriteLog("开始读取会员积分3",3);//[TIME OUT(Second) 0.000065]
				iReturn=GetSalePos()->GetPosMemberInterface()->ReadMemberCardInfo(m_MemberImpl.m_Member.szMemberCode,nUseRemoteMemScore)?1:-1;
				break;
			case 4: //获取会员socket实时积分  mabh 
				GetSalePos()->GetWriteLog()->WriteLog("开始读取会员积分4",3);//[TIME OUT(Second) 0.000065]
				iReturn=GetSalePos()->GetPosMemberInterface()->ReadMemberCardInfo_socket(m_MemberImpl.m_Member.szMemberCode,nUseRemoteMemScore)?1:-1;
				break;

			case 0: //获取会员本地积分 //mabh 
				return 1;
		}
	}
	if (iReturn!=1)//读取会员积分失败
	{
		//ClearCustomerInfo();//aori add 2011-11-21 13:55:42 会员卡输入超时 无反应	
		return -2;	
	}*/
//	return 1;
}

//返回当前的销售流水
//##ModelId=3E641FAB0177
int CSaleImpl::GetSaleID()
{
	return m_nSaleID;
}
//返回当前的销售流水
//##ModelId=3E641FAB0177
SYSTEMTIME CSaleImpl::GetSaleDT()
{
	return m_stSaleDT;
}


//======================================================================
// 函 数 名：InitialNextSale()
// 功能描述：初始化下一笔销售
// 输入参数：
// 输出参数：bool
// 创建日期：00-2-21
// 修改日期：2009-4-1 曾海林
// 修改日期：2009-4-6 曾海林 删除促销零时表
// 作  者：***
// 附加说明：数据库变更修改 Sale_Temp
//======================================================================
void CSaleImpl::InitialNextSale(SaleFinishReason FinishReaseon,int saleid)
{//aori change 2011-6-10 11:09:25 proj2011-6-10-1:生鲜挂起db错误

	Clear();
	m_nSaleID=m_pADB->GetNextSaleID();	
	m_vecSaleMerch.clear();//	m_VectLimitInfo.clear();//m_vecBeforReMem.clear();//aori add 2012-1-19 18:29:09proj解挂错误 ： 先后刷会员卡卖一笔 ，再 挂起一笔 ，然后解挂 出错
	m_ServiceImpl.Clear();
	m_nDiscountStyle = SDS_NO;
	m_nMemDiscountStyle = SDS_NO;
	m_fDiscount = 0.0;
	m_fMemDiscount = 0.0;
	m_fDiscountRate = 100.0;
	m_fMemDiscountRate = 100.0;
	m_bCanChangePrice = false;
	//初始电商变量
	//m_EorderID = "";
	//m_EMemberID = "";
	//m_EMOrderStatus = 0;
	ClearCashier(m_tSalesman);
	GetLocalTimeWithSmallTime(&m_stSaleDT);
	memcpy(&m_stCheckSaleDT, &m_stSaleDT, sizeof(SYSTEMTIME));
	memcpy(&m_stEndSaleDT, &m_stSaleDT, sizeof(SYSTEMTIME));//TODO: 保存到临时销售主表
	CPromotionInterface *pPromotion=GetSalePos()->GetPromotionInterface();//促销初始化//int iPromotionReturn=0;CString strPromotionMsg="";//aori del nouse optimize 2011-10-10 11:31:11 nouse
	if (pPromotion!=NULL&&pPromotion->m_bValid)
	{
		pPromotion->Init();
	}

//*******************************************
	//if(FinishReaseon==sale)DelTempSaleDB();2013-2-4 15:48:20 proj2013-1-17 优化 saleitem 重启丢商品
	CAutoDB db(true);
	CString strSQL;
	switch(FinishReaseon)
	{//2013-2-4 17:17:44 proj2013-1-17-4 优化 saleitem 取消相同商品时sql错误
		case SaleFinish_OnInputFirstMerch:
			strSQL.Format("delete from Sale_Temp where HumanID=%d ", GetSalePos()->GetCasher()->GetUserID());
			db.Execute(strSQL);
			strSQL.Format("INSERT INTO Sale_Temp(SaleDT,HumanID,MemberCode,Shift_No,MemCardNO,MemCardNOType,EOrderID,EMemberID,EMOrderStatus) VALUES"
				"('%s',%d,%s,%d,'%s',%d,'%s','%s','%d')", GetFormatDTString(m_stSaleDT), 
				GetSalePos()->GetCasher()->GetUserID(),GetFieldStringOrNULL(m_MemberImpl.m_Member.szMemberCode),m_uShiftNo,m_MemberImpl.m_Member.szInputNO,m_MemberImpl.m_Member.nCardNoType,m_EorderID,m_EMemberID,m_EMOrderStatus);
			db.Execute(strSQL);//aori change 2011-3-22 16:51:36 优化clientdb ：删除execute2

			break;
		case SaleFinish_CancelSale:
			m_nSaleID--;
			DelTempSaleDB();
			ClearCustomerInfo();
			GetSalePos()->GetPaymentInterface()->ArrayBill.RemoveAll(); //thinpon. 需要签购单的支付初始化变数
			break;
		case SaleFinish_poweron:
				if ( HasUncompletedSale() ) 
				{
					LoadUncompleteSale();
					//saledlg.m_uShiftNo=m_uShiftNo;2013-2-16 14:59:39 proj2013-2-16-1 优化 下笔销售获取未完成销售的班次
					//thinpon. 2008.08.19   为了下笔流水加1，标志置为否
					//m_bNeedAutoAdd = TRUE;//FALSE aori optimize 2011-10-10 13:28:07 改为bool 并名称、逻辑同时取反
				}
				else 
				{
					GetShiftNO();//saledlg.m_uShiftNo = GetShiftNO();2013-2-16 14:59:39 proj2013-2-16-1 优化 下笔销售
					UpdateShiftBeginTime();
					ClearCustomerInfo();
					GetSalePos()->GetPaymentInterface()->ArrayBill.RemoveAll();	//thinpon. 需要签购单的支付初始化变数
				}
			break;
		case SaleFinish_UnHangupSale:
				CUnHangUpDlg* dlg= new CUnHangUpDlg();
				if ( dlg->DoModal() == IDOK )
				{
					if ( dlg->m_bUnHangup == TRUE ) 
					{
						UnHangupSale( dlg->m_nlsh, &dlg->m_stTime) ;						
					}
				}
				delete dlg; 
			break;
	}
	m_bSaveDataSucess=false;

	CString strLogBuf="";
	strLogBuf.Format("销售开始 流水:%d 收银:%d 班次:%d 门店:%d pos号:%d",m_nSaleID,
		GetSalePos()->GetCasher()->GetUserID(),m_uShiftNo,GetSalePos()->GetParams()->m_nOrgNO,
		GetSalePos()->GetParams()->m_nPOSID);
	GetSalePos()->GetWriteLog()->WriteLog(strLogBuf,1,POSLOG_LEVEL1);	
}
void CSaleImpl::DelTempSaleDB()
{//2013-2-4 15:48:20 proj2013-1-17 优化 saleitem 重启丢商品
	CString strSQL;
	CAutoDB db(true);
	int nCashierID = GetSalePos()->GetCasher()->GetUserID();
	
		db.m_pDB->BeginTrans();

		strSQL.Format("DELETE FROM Sale_Temp WHERE HumanID = %d", nCashierID);
		db.Execute(strSQL); 

		strSQL.Format("DELETE FROM SaleItem_Temp WHERE HumanID = %d", nCashierID);
		check_lock_saleitem_temp(); 
		db.Execute(strSQL); 

		strSQL.Format("DELETE FROM SaleItemPromotion_Temp");
		db.Execute(strSQL); 

		strSQL.Format("delete PaymentError_Temp");
		db.Execute(strSQL); 

		if(!m_PayImpl.DelAllTempPayment(db.m_pDB))
			db.m_pDB->RollbackTrans();
		else
			if(!GetSalePos()->GetPromotionInterface()->DeletePromotionTemp(db.m_pDB))
				db.m_pDB->RollbackTrans();
			else
				db.m_pDB->CommitTrans();

	CUniTrace::Trace("CSaleImpl::DelTempSaleDB()");
}
//
//======================================================================
// 函 数 名：SetCustomerChar
// 功能描述：设置本笔销售的顾客特征
// 输入参数：
// 输出参数：bool
// 创建日期：00-2-21
// 修改日期：2009-4-1 曾海林
// 作  者：***
// 附加说明：数据库变更修改 Sale_Temp
//======================================================================
void CSaleImpl::SetCustomerChar(const char *szCustomerCharCode)
{
	m_strCustomerCharacter = szCustomerCharCode;

	//记录
	CPosClientDB* pDB = CDataBaseManager::getConnection(true);
	if ( !pDB ) {
		CUniTrace::Trace("CSaleImpl::SetCustomerChar");
		CUniTrace::Trace(MSG_CONNECTION_NULL, IT_ERROR);
		return;
	}

	CString strSQL;
	strSQL.Format("UPDATE Sale_Temp SET CharCode='%s' WHERE SaleDT = '%s' AND HumanID = %d", 
		m_strCustomerCharacter.c_str(), 
		GetFormatDTString(m_stSaleDT), 
		GetSalePos()->GetCasher()->GetUserID());
	pDB->Execute2(strSQL);//aori change 2011-3-22 16:51:36 优化clientdb ：删除execute2
	CDataBaseManager::closeConnection(pDB);
}

//初始化SaleImpl，只需调用一次
//======================================================================待
// 函 数 名：Initialize()
// 功能描述：
// 输入参数：
// 输出参数：bool
// 创建日期：00-2-21
// 修改日期：2009-4-2 曾海林
// 作  者：***
// 附加说明：促销变更修改,重新整理
//======================================================================/
void CSaleImpl::Initialize()
{
	m_pADB = &GetSalePos()->m_pADB;
	m_pParam = GetSalePos()->GetParams();
	m_nSaleID = m_pADB->GetNextSaleID();
	if ( m_nSaleID == 1 )
		m_nSaleID = GetSalePos()->GetParams()->m_nInitSaleID;
	m_PayImpl.Initialize();
	//<--#服务调用接口#
	m_ServiceImpl.Initialize();
	if(m_ServiceImpl.HasMerchSVConfig())
		GetSalePos()->GetPaymentInterface()->m_ordinaryMarchMapPS=
			m_ServiceImpl.DelPosMobilePaymentStyle(&GetSalePos()->GetPaymentInterface()->m_ordinaryMarchMapPS);
	m_MemberImpl.Clear();

	//***************************************************
	//促销初始化
    CPromotionInterface *pPromotion=GetSalePos()->GetPromotionInterface();
	int iPromotionReturn=0;
	CString strPromotionMsg="";
	//返回参数说明(红利提示2，换购提示3，正常结束1，错误提示-1)
	if (pPromotion!=NULL&&pPromotion->m_bValid){
		pPromotion->Init();
	}
	m_uShiftNo=GetShiftNO();
	GetSalePos()->GetPaymentInterface()->ArrayBill.RemoveAll();

	//初始电商变量
	m_EorderID = "";
	m_EMemberID = "";
	m_EMOrderStatus = 0;

	m_bSaveDataSucess=false;
}
void CSaleImpl::GetRtnID()
{
	///给退货用的,为了尽量减少退货的重号
	SYSTEMTIME stNow;
	GetLocalTimeWithSmallTime(&stNow);
	m_nRtnID = m_pADB->GetNextReturnID(&stNow);
}




//======================================================================
// 函 数 名：Save()
// 功能描述：保存销售数据(包括保存销售、明细、促销,支付信息、打印小票)
// 输入参数：
// 输出参数：void
// 创建日期：2009-4-6 
// 修改日期：
// 作  者：曾海林
// 附加说明：增加促销信息保存
//======================================================================
bool CSaleImpl::Save()
{
	GetSalePos()->GetWriteLog()->WriteLog("[FUN BEGIN: CSaleImpl::Save()]",3,POSLOG_LEVEL3); //解决24小时问题，如果跨天需要重新获得时间//if(!IsInSameDay(m_stEndSaleDT, m_stSaleDT))
	GetLocalTimeWithSmallTime(&m_stEndSaleDT);
	if(!IsInSameDay(m_stEndSaleDT, m_stSaleDT)&&!m_pADB->IsSaleDataExist(&m_stEndSaleDT))	
		m_nSaleID = m_pADB->GetNextSaleID(&m_stEndSaleDT);
	if (abs(GetSalePos()->GetPromotionInterface()->m_fDisCountAmt)>=0.1)
		GetSalePos()->GetPromotionInterface()->WritePromotionLog(m_nSaleID);//写促销日至//aori move from below 2011-8-7 8:38:11  proj2011-8-3-1 用 mutex等判断两个c//GetSalePos()->GetWriteLog()->WriteLog("[FUN TRACE: CSaleImpl::Save()]->call IsSaleDataExist() over",3,POSLOG_LEVEL4); 	//aori del 促销日志//CUniTrace::Trace("--------保存到正式表前 各表内容:--!!!---------");GetSalePos()->GetPromotionInterface()->WritePromotionLog(m_nSaleID);CUniTrace::Trace("----------------------------------------------");//aori add 2011-8-3 17:03:07 proj2011-8-3-1 销售不平 条件促销 临时表 记日志里 
	CPosClientDB *pDB = CDataBaseManager::getConnection(true);	
	if ( !pDB ) 
	{
		GetSalePos()->GetWriteLog()->WriteLog("[FUN TRACE: CSaleImpl::Save()]->getConnection 失败->return false ",3,POSLOG_LEVEL1); 
		return false;
	}
	//GetSalePos()->GetWriteLog()->WriteLog("[FUN TRACE: CSaleImpl::Save()]->getConnection success->return true ",3,POSLOG_LEVEL4); 
	pDB->BeginTrans();
	bool bRet = false;
	if ( m_PayImpl.SavePayment(pDB, m_stEndSaleDT, m_nSaleID, GetBNQTotalAmt()) )
	{
		GetSalePos()->GetWriteLog()->WriteLog("[FUN TRACE: CSaleImpl::Save()]->支付信息保存成功",3,POSLOG_LEVEL3);//normal
		//if(m_MemberImpl.Save(pDB, GetTotalAmt())){
		//	GetSalePos()->GetWriteLog()->WriteLog("[FUN TRACE: CSaleImpl::Save()]->会员信息保存结束",3,POSLOG_LEVEL3);//normal
			if ( Save(pDB, m_nSaleID, SS_NORMAL) )
			{
				GetSalePos()->GetWriteLog()->WriteLog("[FUN TRACE: CSaleImpl::Save()]->销售商品信息保存成功",3,3);//normal
				if (1==GetSalePos()->GetPromotionInterface()->SavePromotion(m_nSaleID,m_stEndSaleDT,pDB))
				{//pDB,aori change 2011-8-7 8:38:11  proj2011-8-3-1 B sql卡住不返回问题保存促销信息{GetSalePos()->GetWriteLog()->WriteLog("[FUN TRACE: CSaleImpl::Save()]->调用促销信息保存结束",3,POSLOG_LEVEL3);//normal
					bRet = true;//红利积分不为0时，扣减会员积分//清起始SaleID
					if ((int)GetPrivateProfileInt( SECTION_POSCLIENT, "InitSaleID", 1, CString(GetSalePos()->GetAppPath()) + POS_INI_FILE ) != 1 )
						WritePrivateProfileString( SECTION_POSCLIENT, "InitSaleID","1", CString(GetSalePos()->GetAppPath()) + POS_INI_FILE);									
				}
			}
		//}	
	}	
	if(bRet)
	{
		pDB->CommitTrans();	
		CDataBaseManager::closeConnection(pDB);	
		GetSalePos()->GetWriteLog()->WriteLog("交易数据保存成功",3,POSLOG_LEVEL1);//normal
	} 
	else 
	{ 
		pDB->RollbackTrans();	
		CDataBaseManager::closeConnection(pDB);	
		GetSalePos()->GetWriteLog()->WriteLog("交易数据保存失败",5,POSLOG_LEVEL1);GetSalePos()->GetWriteLog()->WriteLog("[FUN EXIT: CSaleImpl::Save()]->bRet=false->返回值:false",3,POSLOG_LEVEL1); //CUniTrace::Trace("--------保存到正式表后 各表内容:--!!!---------");GetSalePos()->GetPromotionInterface()->WritePromotionLog(m_nSaleID);CUniTrace::Trace("----------------------------------------------");//aori add 2011-8-3 17:03:07 proj2011-8-3-1 销售不平 条件促销 临时表 记日志里 
		return false;
	}
	bool isEMSale= IsEMSale();
	m_bSaveDataSucess=true;//红利积分不为0时，扣减会员积分 //mabh
	if (GetSalePos()->GetPromotionInterface()->m_nHotReducedScore>0)
	{
		GetSalePos()->GetWriteLog()->WriteLog("调用红利减积分",3,POSLOG_LEVEL1);//normal
		//Checkbonussales();
		//Getbonusvalid();
		GetSalePos()->GetPosMemberInterface()->SetSaleImpl(this);
		if (1== GetSalePos()->GetPosMemberInterface()->UpdateMemberScore(m_nSaleID)) 
			GetSalePos()->GetWriteLog()->WriteLog(CString("更新charcode")+((upd_CharCode() == 1)?"成功":"失败"),3,POSLOG_LEVEL2);
	}//税控调用//aori begin 2011-12-30 14:42:38 proj 2011-12-22-1 combine 2.75 with 2.71 saleimpl
	if (GetSalePos()->GetParams()->m_nPrintTaxInfo == 1)
	{
 		GetSalePos()->GetWriteLog()->WriteLog("开始税控发票调用",3);//写日志
		CTaxInfoImpl *TaxInfoImpl=GetSalePos()->GetTaxInfoImpl();
		TaxInfoImpl->SetSaleImpl(this);
		TaxInfoImpl->SetPayImpl(&m_PayImpl);
		TaxInfoImpl->SaleTaxBilling();
		GetSalePos()->GetWriteLog()->WriteLog("结束税控发票调用",3);//写日志
	}//aori end 2011-12-30 14:42:38 proj 2011-12-22-1 combine 2.75 with 2.71 saleimpl
	//海信实时积分调用
	//GetSalePos()->GetWriteLog()->WriteLog("开始海信实时积分调用0",3);//写日志
	if (GetSalePos()->GetParams()->m_bRealtimeJF == 1 && strlen(m_MemberImpl.m_Member.szMemberCode) > 1)
	{
 		GetSalePos()->GetWriteLog()->WriteLog("开始海信实时积分调用",3);//写日志
		//RealtimeJF  *m_RealtimeJF = GetSalePos()->GetRealtimeJF() ;
		//bool rtn = m_RealtimeJF->UpRealtimeJF(m_nSaleID);
		CString saledate=::GetFormatDTString(m_stEndSaleDT,true);		
		bool rtn = m_MemberImpl.RealtimeScore(saledate,m_nSaleID);
		if (rtn)
		{
 			GetSalePos()->GetWriteLog()->WriteLog("海信实时积分成功！",3);//写日志
		}
		else
		{
 			GetSalePos()->GetWriteLog()->WriteLog("海信实时积分失败！",3);//写日志
			//m_RealtimeJF->Upd_Saleinfo(m_nSaleID,0,0);
			//m_MemberImpl.UpdSaleScore(m_nSaleID,0,0);
		}
		GetSalePos()->GetWriteLog()->WriteLog("结束海信实时积分调用",3);//写日志
	}

	//更新电商会员订单状态
	//m_pSaleImpl->m_EorderID = "1110";m_pSaleImpl->m_EMeberID = "123456";//test

	CString msg;
	msg.Format("判断是否电商交易:m_EorderID=%s, m_EMemberID=%s",m_EorderID,m_EMemberID);
	CUniTrace::Trace(msg);

	if (isEMSale) 
	{
			//上传电商多点订单
		if (!m_PayImpl.CheckDmalPay() && GetSalePos()->GetParams()->m_bDmallOnlinePay)
		{
			//记录日志
			GetSalePos()->GetWriteLog()->WriteLog("电商数据传入:开始",4,POSLOG_LEVEL2);//[TIME OUT(Second) 0.000065]
			CEorderinterface* m_eorderinterface = GetSalePos()->GetEorderinterface();
			bool rtn=false;
			if(m_eorderinterface!=NULL && m_eorderinterface->m_bValid)
			{
				m_eorderinterface->SetSaleImpl(this);
				GetSalePos()->GetPromotionInterface()->LoadSaleInfoEnd(GetSaleID(),true);//写入m_vecSaleMerchEnd
				try 
				{	
					m_eorderinterface->m_fofflinePayCash = 0.00;
					rtn = m_eorderinterface->Eorder_info(2,11);//按照电商会员订单检查销售数据
					if (rtn)  
					{
						//if(!m_eorderinterface->m_bDowngradeFlag)
						Upd_EMorder_status(EMORDER_STATUS_CREATE,1,m_eorderinterface->m_bDowngradeFlag);
					}
						
					if (!rtn)
					{
						GetSalePos()->GetWriteLog()->WriteLog("电商数据传入:异常！！！",4,POSLOG_LEVEL2);
					}
				}
				catch (CException* e) 
				{ CUniTrace::Trace(*e);
					e->Delete( );		
					GetSalePos()->GetWriteLog()->WriteLog("电商数据传入:失败！！！",4,POSLOG_LEVEL2);
					//MessageBox( "电商数据传入:失败！！！", "提示", MB_OK | MB_ICONWARNING);	
				} 

			}
			else
			{
				if (m_eorderinterface==NULL)
				{
					GetSalePos()->GetWriteLog()->WriteLog("电商数据模块为空！！！",4,POSLOG_LEVEL2);
					//MessageBox( "电商数据模块为空！！！", "提示", MB_OK | MB_ICONWARNING);	
				}
				else
				{
					GetSalePos()->GetWriteLog()->WriteLog("电商数据模块不可用！！！",4,POSLOG_LEVEL2);
					//MessageBox( "电商数据模块不可用！！！", "提示", MB_OK | MB_ICONWARNING);	

				}
			}
		}

		if (GetSalePos()->GetParams()->m_bDmallOnlinePay)
		{
			GetSalePos()->GetWriteLog()->WriteLog("多点支付成功状态回调接口:开始",4,POSLOG_LEVEL2);//[TIME OUT(Second) 0.000065]
		}
		else
		{
			GetSalePos()->GetWriteLog()->WriteLog("更新电商会员订单状态上传接口:开始",4,POSLOG_LEVEL2);//[TIME OUT(Second) 0.000065]
		}
		CEorderinterface* m_eorderinterface = GetSalePos()->GetEorderinterface();
		bool rtn=false;
		if(m_eorderinterface!=NULL && m_eorderinterface->m_bValid)
		{
			try {
				if (GetSalePos()->GetParams()->m_bDmallOnlinePay)
				{
					rtn = m_eorderinterface->Eorder_info(2,13);//多点支付成功状态回调接口
				}
				else
				{
					rtn = m_eorderinterface->Eorder_info(2,3);//更新电商会员订单状态上传
				}
			if (rtn)
				{
					//if(!m_eorderinterface->m_bDowngradeFlag)
					Upd_EMorder_status(EMORDER_STATUS_FINISH,1,m_eorderinterface->m_bDowngradeFlag);
				}
			}catch (CException* e) 
			{ CUniTrace::Trace(*e);
				e->Delete( );		
				if (GetSalePos()->GetParams()->m_bDmallOnlinePay)
				{
					GetSalePos()->GetWriteLog()->WriteLog("多点支付成功状态回调接口:失败！！！",4,POSLOG_LEVEL2);
				}
				else
				{
					GetSalePos()->GetWriteLog()->WriteLog("更新电商会员订单状态上传接口:失败！！！",4,POSLOG_LEVEL2);
				}
			} 
		}
		else
		{
			if (m_eorderinterface==NULL)
				GetSalePos()->GetWriteLog()->WriteLog("电商数据模块为空！！！",4,POSLOG_LEVEL2);
			else
				GetSalePos()->GetWriteLog()->WriteLog("电商数据模块不可用！！！",4,POSLOG_LEVEL2);
		}
		if (GetSalePos()->GetParams()->m_bDmallOnlinePay)
		{
			GetSalePos()->GetWriteLog()->WriteLog("多点支付成功状态回调接口：结束",4,POSLOG_LEVEL2);//[TIME OUT(Second) 0.000065]
		}
		else
		{
			GetSalePos()->GetWriteLog()->WriteLog("更新电商会员订单状态上传接口：结束",4,POSLOG_LEVEL2);//[TIME OUT(Second) 0.000065]
		}
	//电商数据传入：结束

	}
	else//m_EorderID m_EMeberID
	{
		GetSalePos()->GetWriteLog()->WriteLog("非电商销售数据。",4,POSLOG_LEVEL2);
	}

	double fSaleAmt = GetTotalAmt();
	//会员消费送积分，系统参数m_bMemberXFSScore为1时，执行以下代码
	if (GetSalePos()->GetParams()->m_bMemberXFSScore && strlen(m_MemberImpl.m_Member.szMemberCode) > 1)
	{
		CString saledate=::GetFormatDTString(m_stEndSaleDT,true);
		bool bRtn = m_MemberImpl.MemberXFSScore(saledate, m_nSaleID, fSaleAmt);
		if(bRtn)
		{
			GetSalePos()->GetWriteLog()->WriteLog("会员消费送积分成功!", 3);
		}
		else
		{
			GetSalePos()->GetWriteLog()->WriteLog("会员消费送积分失败!", 3);
		}
	}
	//会员抽奖功能，系统参数m_bMemberLottery为1时，执行以下代码
	if (GetSalePos()->GetParams()->m_bMemberLottery && strlen(m_MemberImpl.m_Member.szMemberCode) > 1)
	{
		bool bReturn;
		bReturn = m_MemberImpl.MemberLottery(fSaleAmt);
		if (bReturn)
		{
			GetSalePos()->GetWriteLog()->WriteLog("会员抽奖功能成功!", 3);
		}
		else
		{
			GetSalePos()->GetWriteLog()->WriteLog("会员抽奖功能失败!", 3);
		}
	}
	GetSalePos()->GetPosMemberInterface()->SavePaymentError(m_stEndSaleDT);
	GetSalePos()->m_Task.putSale(m_nSaleID, m_stEndSaleDT);
	GetSalePos()->GetWriteLog()->WriteLog("[FUN END: CSaleImpl::Save()] 返回值:true",3,POSLOG_LEVEL3); 
	return bRet;
}

//返回支付类
//##ModelId=3E641FAB00CD
CPaymentImpl * CSaleImpl::GetPaymentImpl()
{
	return &m_PayImpl;
}
/*
	说明，这里并不合并项，只是把各项的价格和作调整
	该功能用于用户对折扣商品根据该商品的上下限调整其价格
	主要用于用户取消了某项商品，这时候相同PLU的商品的价格可能因为折扣促销的上下限问题
	而调整价格和部分数量，
	如果调整了价格和数量，返回1
	没有调整，返回0;
	
	首先找出该商品序列，算出销售数量总和MergeCount
*/
//##ModelId=3E641FAB0073
unsigned int CSaleImpl::MergeItems(const char *szPLU)//2013-2-27 15:30:11 proj2013-1-23-1 称重商品 加入 会员促销、限购
{
	//首先找出所有未取消并且是折扣销售的商品（包括已经折扣或者因为未满足数量未计算折扣的项,而且必须是单品），
	//同时算出该商品的销售数量总和
	vector<SaleMerchInfo*> vSM;
	int nMergeCount = 0;//2013-2-27 15:30:11 proj2013-1-23-1 称重商品 加入 会员促销、限购

	for ( vector<SaleMerchInfo>::iterator merchInfo = m_vecSaleMerch.begin(); merchInfo != m_vecSaleMerch.end(); ++merchInfo) 
	{
		if ( merchInfo->nMerchState != RS_CANCEL &&
			(merchInfo->nDiscountStyle == DS_ZK || merchInfo->nDiscountStyle == DS_TZ)&&
			strcmp(merchInfo->szPLU, szPLU) == 0 &&
			merchInfo->nManagementStyle == MS_DP && (!merchInfo->bIsDiscountLabel)) 
		{
			nMergeCount += merchInfo->fSaleQuantity/*nSaleCount*/;
			vSM.push_back(merchInfo);
		}
	}

	if ( vSM.size() == 0 )
		return 0;

	int nRestCount = nMergeCount;//2013-2-27 15:30:11 proj2013-1-23-1 称重商品 加入 会员促销、限购
	int nMoreCount = 0;	//每项重新分配后的剩余数
	int nZKCount = 0;//2013-2-27 15:30:11 proj2013-1-23-1 称重商品 加入 会员促销、限购
	int nUpLimit = vSM[0]->nUpLimit;
	short nLowLimit = vSM[0]->nLowLimit;
	double fZKPrice = vSM[0]->fZKPrice;

	SaleMerchInfo* pSMI = NULL;
	for ( vector<SaleMerchInfo*>::iterator itr = vSM.begin(); itr != vSM.end(); ++itr ) 
	{
		pSMI = *itr;
		
		if ( nLowLimit == 0 && nUpLimit == 0) 
		{
			//如果没有下限, 没有上限
			pSMI->fActualPrice = fZKPrice;
			pSMI->nMerchState = RS_DS_ZK;
			nZKCount += pSMI->nSaleCount;
			nRestCount -= pSMI->nSaleCount;
		} 
		else if ( nLowLimit == 0 && nUpLimit > 0) 
		{
			//如果没有下限，有上限
			//如果以及打折的总数还没有超出上限的话
			if ( nZKCount < nUpLimit )
			{
				pSMI->fActualPrice = fZKPrice;
				pSMI->nMerchState = RS_DS_ZK;
				int nSaleCount = pSMI->nSaleCount + nMoreCount;
				pSMI->nSaleCount = __min(nSaleCount, nUpLimit - nZKCount);
				nRestCount -= pSMI->nSaleCount;
				nZKCount += pSMI->nSaleCount;
				nMoreCount = nSaleCount - pSMI->nSaleCount;
			}
			else 
			{
				pSMI->fActualPrice = pSMI->fLabelPrice;
				pSMI->nMerchState = RS_NORMAL;
				pSMI->nSaleCount += nMoreCount;
				nMoreCount = 0;
				nRestCount -= pSMI->nSaleCount;
			}
		}
		else if ( nLowLimit > 0 && nUpLimit == 0)
		{
			//如果有下限没有上限
			if ( nMergeCount < nLowLimit ) 
			{
				pSMI->fActualPrice = pSMI->fLabelPrice;
				pSMI->nMerchState = RS_NORMAL;
				nRestCount -= pSMI->nSaleCount;
			}
			else
			{
				pSMI->fActualPrice = fZKPrice;
				pSMI->nMerchState = RS_DS_ZK;
				nZKCount += pSMI->nSaleCount;
				nRestCount -= pSMI->nSaleCount;
			}
		} 
		else
		{	
			//如果有下限有上限
			//如果总数没有超过下限，都不打折
			if ( nMergeCount < nLowLimit ) 
			{
				pSMI->fActualPrice = pSMI->fLabelPrice;
				pSMI->nMerchState = RS_NORMAL;
				nRestCount -= pSMI->nSaleCount;
			}
			else 
			{
				//如果以及打折的总数还没有超出上限的话
				if ( nZKCount < nUpLimit ) 
				{
					pSMI->fActualPrice = fZKPrice;
					pSMI->nMerchState = RS_DS_ZK;
					int nSaleCount = pSMI->nSaleCount + nMoreCount;
					pSMI->nSaleCount = __min(nSaleCount, nUpLimit - nZKCount);
					nRestCount -= pSMI->nSaleCount;
					nZKCount += pSMI->nSaleCount;
					nMoreCount = nSaleCount - pSMI->nSaleCount;
				} 
				else 
				{
					pSMI->fActualPrice = pSMI->fLabelPrice;
					pSMI->nMerchState = RS_NORMAL;
					pSMI->nSaleCount += nMoreCount;
					nMoreCount = 0;
					nRestCount -= pSMI->nSaleCount;
				}
			}
		}
		ReCalculate(*pSMI);	
	}//	for ( vector<SaleMerchInfo*>

	//如果最后还剩余一点的话，在最后新加一项（这种情况在有上限并且超出上限的部分会剩）
	if ( nMoreCount > 0 ) {
		SaleMerchInfo newSMI=*pSMI;//memcpy(&newSMI, (const void *)pSMI, sizeof(SaleMerchInfo));
		newSMI.nSaleCount = nMoreCount;
		newSMI.fActualPrice = newSMI.fLabelPrice;
		newSMI.nMerchState = RS_NORMAL;
		newSMI.fSaleAmt =newSMI.fSaleAmt_BeforePromotion= newSMI.fActualPrice * newSMI.nSaleCount;//aori add 2012-2-7 16:11:04 proj 2.79 测试反馈：条件促销时 不找零 加 fSaleAmt_BeforePromotion
		newSMI.fSaleDiscount = 0;
		AddItem(newSMI);
	}

	vSM.clear();
	return 1;
}

//取得实际销售项总数，当小于1时，不能开票
//##ModelId=3E641FAB005F
int CSaleImpl::GetSaleItemCount()
{
	int nCount = 0;
	for ( vector<SaleMerchInfo>::iterator merchInfo = m_vecSaleMerch.begin(); merchInfo != m_vecSaleMerch.end(); ++merchInfo ) {
		if ( (merchInfo->nMerchState != RS_CANCEL) 
				&& (merchInfo->fSaleQuantity/*nSaleCount*/ != 0) )
			nCount ++;
	}
	return nCount;
}

//根据szPLU和折扣类型返回第一个相同的项（用于复制）
//原则，
// 1.忽略被取消的商品
// 2.如果参数为印花，则找到相应的印花商品，找不到则返回NULL
// 3.如果是折扣标签，返回NULL
//##ModelId=3E641FAC0273
SaleMerchInfo* CSaleImpl::GetSaleMerchInfo(CString pPLU, short nDiscountStyle)
{
	char* szPLU=(LPSTR)(LPCTSTR)pPLU;
	for ( vector<SaleMerchInfo>::iterator merchInfo = m_vecSaleMerch.begin(); merchInfo != m_vecSaleMerch.end(); ++merchInfo) 
	{
		if ( merchInfo->nMerchState != RS_CANCEL 
			&& merchInfo->nMerchState != RS_COMBINED 
			&& merchInfo->nMerchState != RS_DS_MANUAL 
			&& ( strcmp(merchInfo->szPLUInput, szPLU) == 0 ||
				strcmp(merchInfo->szPLU, szPLU) == 0 ) ) 
		{
			if( merchInfo->bIsDiscountLabel )
				return NULL;
			if ( nDiscountStyle == DS_YH ) 
			{
				if ( (merchInfo->nDiscountStyle & 0xff) == DS_YH )
				{
					return merchInfo;
				}
			} 
			else if ( (merchInfo->nDiscountStyle & 0xff) != DS_YH )
			{
				return merchInfo;
			}
		}
	}
	return 0;	
}
/*
//返回会员的级别
//##ModelId=3E641FAB004B
unsigned char CSaleImpl::GetMemberLevel()
{
	return m_MemberImpl.m_Member.nGradeID;
}

//返回会员编码
//##ModelId=3E641FAB0037
const char * CSaleImpl::GetMemberCode()
{
	return m_MemberImpl.m_Member.szMemberCode;
}

//返回会员名称
//##ModelId=3E641FAA0389
const char * CSaleImpl::GetMemberName()
{
	return m_MemberImpl.m_Member.szMemberName;
}

const double CSaleImpl::GetMemberTotIntegral()
{
	return m_MemberImpl.m_Member.dTotIntegral;
}

//返回会员卡类型
const short  CSaleImpl::GetMemberCardType()
{
	return m_MemberImpl.m_Member.nCardStyle;
}
*/

//根据折扣上下限来拆分商品，拆出应该多少数量的商品进行折扣促销
//##ModelId=3E641FAC01B5
int CSaleImpl::GetZKCount(int nSaleCount, short nLowLimit, int nUpLimit)
{
	if ( nLowLimit == 0 ) {
		if ( nUpLimit == 0 ) {
			return nSaleCount;
		} else {
			return __min(nSaleCount, nUpLimit);
		}
	} else {
		if ( nUpLimit == 0 ) {
			return nSaleCount < nLowLimit ? 0 : nSaleCount;
		} else {
			return (nSaleCount < nLowLimit ? 0 : __min(nSaleCount, nUpLimit));
		}
	}
}

//在开票前，将单品类的商品需要合并得合并起来
//从后边遍历商品
// 曾海林 20090515 修改合并条件和合并项目，为促销服务
//##ModelId=3E641FAB0023
void CSaleImpl::CombineItems()
{
	vector<SaleMerchInfo>::iterator ritr = m_vecSaleMerch.end();
	do {
		ritr --;
		for ( vector<SaleMerchInfo>::iterator itr = m_vecSaleMerch.begin(); itr != m_vecSaleMerch.end(); ++ itr ) 
		{
			if ( itr >= ritr )
				break;
			if ( strcmp(ritr->szPLU, itr->szPLU) == 0 
				&& ritr->nManagementStyle == MS_DP
				&& ritr->nMerchState != RS_CANCEL
				&& itr->nMerchState != RS_CANCEL
				&& ritr->nMerchState != RS_DS_MANUAL
				//&& ritr->nDiscountStyle == itr->nDiscountStyle
				&& ritr->nMerchState == itr->nMerchState 
				&& ritr->tSalesman.nHumanID == itr->tSalesman.nHumanID
				//&& ritr->bMemberPromotion == itr->bMemberPromotion
			&& !ritr->bIsDiscountLabel && !itr->bIsDiscountLabel
			&& ritr->bLimitSale==itr->bLimitSale//aori add for limitsale
			&& ritr->LimitStyle==itr->LimitStyle//aori add for limitsale
			&& ritr->strOrderNo==itr->strOrderNo//add by wjx
			&& ritr->strOrderItemID == itr->strOrderItemID//add by wjx
			)
			{
				//&& !(ritr->bIsDiscountLabel - itr->bIsDiscountLabel)) {
				//合并之
				double actualPrice = 0;
				itr->nSaleCount += ritr->nSaleCount;
				itr->fSaleDiscount += ritr->fSaleDiscount;
				itr->fSaleAmt += ritr->fSaleAmt;
				itr->fSaleAmt_BeforePromotion=itr->fSaleAmt;//aori add 2012-2-7 16:11:04 proj 2.79 测试反馈：条件促销时 不找零 加 fSaleAmt_BeforePromotion
				
				//合并fSaleQuantity，因为数量记在fSaleQuantity中 [dandan.wu 2016-4-19]
				itr->fSaleQuantity += ritr->fSaleQuantity;
				actualPrice = itr->fSaleAmt/itr->fSaleQuantity;
// 				if ( itr->nMerchType == OPEN_STOCK_2 || itr->nMerchType == OPEN_STOCK_5 )
// 				{
// 					itr->fSaleQuantity += ritr->fSaleQuantity;
// 					actualPrice = itr->fSaleAmt/itr->fSaleQuantity;
// 				}
// 				else
// 				{
// 					actualPrice = itr->fSaleAmt/itr->nSaleCount;
// 				}

				if (itr->scantime < ritr->scantime)
				{
					itr->scantime = ritr->scantime;
				}

				if (itr->IncludeSKU > ritr->IncludeSKU)
				{
					itr->IncludeSKU = ritr->IncludeSKU;
				}

				itr->fLabelPriceNORMAL = actualPrice;
				itr->fRetailPrice = actualPrice;
				itr->fRetailPrice2 = actualPrice;
				itr->fSysPrice = actualPrice;
				itr->fLabelPrice = actualPrice;
				itr->fActualPrice = actualPrice;
				//itr->fMemSaleDiscount += ritr->fMemSaleDiscount;
				ritr->nSaleCount = 0;
				ritr->fSaleDiscount = 0.0f;
				ritr->fSaleAmt = ritr->fSaleAmt_BeforePromotion= 0.0f;
				ritr->nSID = -1; //标识要删除
				m_vecSaleMerch.erase(ritr);
				break;
			}				
		}
	} while ( ritr != m_vecSaleMerch.begin() );

	//重新编号nSID
	int nSID = 0;
	for ( ritr = m_vecSaleMerch.begin(); ritr != m_vecSaleMerch.end(); ++ritr ) {
		ritr->nSID = nSID++;
	}
}

/*
	如果用户按了开票键，已经进行过组合运算，则需要把该组合的其他商品取消组合，其他商品的价格需要重新计算
	并返回vector<int> nSID序列，以便界面更新
*/
//======================================================================待
// 函 数 名：CancelCombine
// 功能描述：
// 输入参数：
// 输出参数：bool
// 创建日期：00-2-21
// 修改日期：2009-4-2 曾海林
// 作  者：***
// 附加说明：促销变更修改,需要重新整理
//======================================================================/
// void CSaleImpl::CancelCombine(int nSID, vector<int>& vOtherCombines)
// {
// 	SaleMerchInfo* merchInfo = &m_vecSaleMerch[nSID];
// 	vOtherCombines.clear();
// 
// 	for ( vector<SaleMerchInfo>::iterator itr = m_vecSaleMerch.begin(); itr != m_vecSaleMerch.end(); ++ itr ) {
// 		if (itr->nSID != merchInfo->nSID
// 			&& itr->nMerchState == RS_COMBINED 
// 			&& itr->nDiscountStyle == merchInfo->nDiscountStyle
// 			&& itr->nMakeOrgNO == merchInfo->nMakeOrgNO 
// 			&& itr->nSalePromoID == merchInfo->nSalePromoID) {
// 			vOtherCombines.push_back(itr->nSID);
// 			//重新计算明细
// 
// 			//取促销价（如果有的话）
// 			itr->nMerchState = RS_NORMAL;
// 			//m_promotion.GetPromotionPrice(*itr, GetMemberLevel());
// 			ReCalculate(*itr);
// 			//MergeItems(itr->szPLU);
// 		}
// 	}	
// }

//##ModelId=3E641FAA03E3
bool CSaleImpl::CanChangePrice()
{	//SaleMerchInfo.m_saleItem.CanChangePrice()
	return m_vecSaleMerch.back().m_saleItem.CanChangePrice();
	return m_bCanChangePrice;
}

//##ModelId=3E641FAA03B1
void CSaleImpl::SetWholeDiscount(short nDiscountStyle, double fDiscountRate, double fDiscount, short nMemDiscountStyle, double fMemDiscountRate, double fMemDiscount)
{
	m_nDiscountStyle = nDiscountStyle;
	m_nMemDiscountStyle = nMemDiscountStyle;

	m_fDiscountRate = fDiscountRate;
	m_fMemDiscountRate = fMemDiscountRate;

	m_fDiscount = fDiscount;
	m_fMemDiscount = fMemDiscount;
}

//##ModelId=3E641FAA03A7
void CSaleImpl::ClearCustomerInfo()
{
	m_strCustomerCharacter = "";
	m_MemberImpl.Clear();
	//初始电商变量
	m_EorderID = "";
	m_EMemberID = "";
	m_EMOrderStatus = 0;
}

//检查，如果支付金额比应收金额少的话，自动加上少的部分
//##ModelId=3E641FAA039D
bool CSaleImpl::CheckPayAmt()
{
	double dSaleTotalAmt = GetBNQTotalAmt();
	double dPayTotalAmt = 0.0;

	for ( vector<PayInfo>::const_iterator pay = m_PayImpl.m_vecPay.begin(); pay != m_PayImpl.m_vecPay.end(); ++pay ) {
		dPayTotalAmt += pay->fPayActualAmt;
	}
	ReducePrecision(dPayTotalAmt);
	if ( dPayTotalAmt != dSaleTotalAmt ) {
		CString strBuf;
		strBuf.Format("收银员: %s[%s] 销售总额 %0.2f 支付总额 %.2f", 
			GetSalePos()->GetCasher()->GetUserName(), 
			GetSalePos()->GetCasher()->GetUserCode(),
			dSaleTotalAmt, dPayTotalAmt );
		m_PayImpl.ReduceChange(dPayTotalAmt - dSaleTotalAmt);
		CUniTrace::Trace(strBuf);
	}
	return true;
}


//##ModelId=3E641FAA0375
void CSaleImpl::SetCheckSaleDT()
{
	GetLocalTimeWithSmallTime(&m_stCheckSaleDT);
}

//======================================================================
// 函 数 名：SaveItemToTemp
// 功能描述：保存商品信息
// 输入参数：int nSID
// 输出参数：bool
// 创建日期：00-2-21
// 修改日期：2009-3-31 曾海林
// 作  者：***
// 附加说明：数据库变更修改（去掉DiscountStyle,SalePromoID,LowLimit,UpLimit,Shift_No)
//fZKPrice 折扣价 DiscountPrice
//SysPrice	原售价		NormalPrice
//ActualPrice系统售价		RetailPrice
//SID  销售ID  SaleID
//ItemID,ScanPLU 改为空
//2015-3-24 新曾电商字段 m_EOrderID m_EMemberID m_EMOrderStatus
//======================================================================
bool CSaleImpl::SaveItemToTemp(int nSID)
{
	if ( nSID >= m_vecSaleMerch.size() )
		return false;

	CPosClientDB* pDB = CDataBaseManager::getConnection(true);
	if ( !pDB ) {
		CUniTrace::Trace("CSaleImpl::SaveItemToTemp");
		CUniTrace::Trace(MSG_CONNECTION_NULL, IT_ERROR);
		return false;
	}
    
	SaleMerchInfo* merchInfo = &m_vecSaleMerch[nSID];
	check_lock_saleitem_temp();//aori later 2011-8-7 8:38:11  proj2011-8-3-1 A 用 mutex等判断两个checkout之间 是否出现了update事件
	try{//CUniTrace::Trace("!!!将对SaleItem_Temp进行插入");//aori add 2011-8-3 17:03:07 proj2011-8-3-1 销售不平 条件促销 临时表 记日志里 
		CString strSQL;
		strSQL.Format("INSERT INTO SaleItem_Temp(SaleDT,HumanID,SaleID,PLU,SimpleName,"
			"ManagementStyle,NormalPrice,LablePrice,RetailPrice,SaleCount,"
			"SaleAmt,SaleDiscount,MakeOrgNO,MerchState,DiscountPrice,"
			"MerchID, SalesHumanID,SaleQuantity,ISFixedPriceAndQuantity,itemID,ScanPLU,ClsCode,DeptCode,promoID,PromoStyle, "//thinpon ,IsDiscountLabel, BackDiscount)"
			"LimitStyle,LimitQty,CheckHistory,InputSec,IncludeSKU,BoxPrice,ItemType,OrderID,CondPromDiscount,ArtID,\
			OrderItemID,ManualDiscount)"
			//aori add for limit sale syn 2010-12-24 9:44:56 加新字段 CheckHistory 同步 saleitem_temp
			// InputSec 扫描时间
			"VALUES ('%s',%d,%d,'%s','%s',%d,%.4f,%.4f,%.4f,%d,%.4f,%.4f,%d,%d,%.4f,%d,%s,%.4f,%d,%d,'%s','%s','%s',%d,%d,%d,%.4f,%d,%d,%d,%4f,%d,'%s',%.4f,'%s',\
			'%s',%.4f)",//%d to %4f2013-2-27 15:30:11 proj2013-1-23-1 称重商品 加入 会员促销、限购
			GetFormatDTString(m_stSaleDT), GetSalePos()->GetCasher()->GetUserID(),
			1, merchInfo->szPLU, merchInfo->szSimpleName,
			merchInfo->nManagementStyle, merchInfo->fSysPrice, merchInfo->fLabelPrice,
			merchInfo->fActualPrice, merchInfo->nSaleCount, 
			merchInfo->fSaleAmt,merchInfo->fSaleDiscount, 
			merchInfo->nMakeOrgNO, merchInfo->nMerchState, merchInfo->fZKPrice,
			merchInfo->nMerchID,
			GetFieldStringOrNULL(merchInfo->tSalesman.nHumanID, INVALIDHUMANID),
			merchInfo->fSaleQuantity,merchInfo->bFixedPriceAndQuantity ,nSID,merchInfo->szPLUInput,merchInfo->szClsCode,merchInfo->szDeptCode,merchInfo->nPromoID,merchInfo->nPromoStyle,
			merchInfo->LimitStyle,merchInfo->LimitQty,merchInfo->CheckedHistory,merchInfo->scantime,
			merchInfo->IncludeSKU,merchInfo->fActualBoxPrice,
			merchInfo->nItemType, merchInfo->strOrderNo, merchInfo->dCondPromDiscount,

			merchInfo->strArtID,merchInfo->strOrderItemID,merchInfo->fManualDiscount); 
			
		pDB->Execute2(strSQL);//aori change 2011-3-22 16:51:36 优化clientdb ：删除execute2
		CDataBaseManager::closeConnection(pDB);CHECKEDHISTORY_NoCheck;
		//CUniTrace::Trace("proj 2011-8-3-1 insert into saleitem_temp");//aori add proj 2011-8-3-1 销售不平 2011-10-27 11:33:33 补充日志
		return true;
	}catch(CException* pe) {
		GetSalePos()->GetWriteLog()->WriteLogError(*pe);
		pe->Delete();
		CDataBaseManager::closeConnection(pDB);
		return false;
	}
}

//======================================================================
// 函 数 名：SaveItemToTemp
// 功能描述：保存商品信息
// 输入参数：int nSID
// 输出参数：bool
// 创建日期：00-2-21
// 修改日期：2009-4-1 曾海林
// 作  者：***
// 附加说明：数据库变更修改（去掉DiscountStyle,SalePromoID,LowLimit,UpLimit,Shift_No)
//fZKPrice 折扣价 DiscountPrice
//SysPrice	原售价		NormalPrice
//ActualPrice系统售价		RetailPrice
//SID  销售ID  SaleID
//ItemID,ScanPLU 改为空   
//SaleItem_Temp
//======================================================================
bool CSaleImpl::SaveItemsToTemp()
{
	CPosClientDB* pDB = CDataBaseManager::getConnection(true);
	if ( !pDB ) {
		CUniTrace::Trace("CSaleImpl::SaveItemToTemp");
		CUniTrace::Trace(MSG_CONNECTION_NULL, IT_ERROR);
		return false;
	}

	CString strSQL;
	CString strSaleDT = GetFormatDTString(m_stSaleDT);
	int nHumanID = GetSalePos()->GetCasher()->GetUserID();

	try
	{
		strSQL.Format("DELETE FROM SaleItem_Temp WHERE SaleDT = '%s' AND HumanID = %d",//aori 为什么清空原来的数据改一下itemid和salehumanid就行 2011-9-16 10:24:26 proj 2011-9-16-1 2011-9-16 10:26:42 会员+100401。1+282701。60+103224.5 + 取消100401时 限购的103224出错 
			strSaleDT, nHumanID);
		pDB->Execute2(strSQL);//aori change 2011-3-22 16:51:36 优化clientdb ：删除execute2
		int iItem=0;
		check_lock_saleitem_temp();//pDB->Execute("select ");//aori test//aori later 2011-8-7 8:38:11  proj2011-8-3-1 A 用 mutex等判断两个checkout之间 是否出现了update事件
		//CUniTrace::Trace("!!!将对SaleItem_Temp进行插入");//aori add 2011-8-3 17:03:07 proj2011-8-3-1 销售不平 条件促销 临时表 记日志里 
		for ( vector<SaleMerchInfo>::iterator merchInfo = m_vecSaleMerch.begin(); merchInfo != m_vecSaleMerch.end(); ++ merchInfo ) 
		{
			//合并商品后，若打印发票，将商品属于哪张发票保存到SaleItem_Temp，因为ilog重新计算后，m_vecSaleMerch从SaleItem_Temp加载信息 [dandan.wu 2016-10-14]
			if ( GetSalePos()->m_params.m_bAllowInvoicePrint )
			{
				strSQL.Format("INSERT INTO SaleItem_Temp(SaleDT,HumanID,SaleID,PLU,SimpleName,"
					"ManagementStyle,NormalPrice,LablePrice,RetailPrice,SaleCount,"
					"SaleAmt,SaleDiscount,MakeOrgNO,MerchState,DiscountPrice,"
					"MerchID, SalesHumanID,SaleQuantity,ISFixedPriceAndQuantity,itemID,ScanPLU,ClsCode,DeptCode,promoID,PromoStyle "//thinpon ,IsDiscountLabel, BackDiscount)"
					",LimitStyle,LimitQty,CheckHistory,InputSec,IncludeSKU,BoxPrice,ItemType,OrderID,CondPromDiscount,ArtID,OrderItemID,ManualDiscount"
					",InvoiceEndNo)"//aori add for limit sale syn 2010-12-24 9:44:56 加新字段 CheckHistory 同步 saleitem_temp
					"VALUES ('%s',%d,%d,'%s','%s',%d,%.4f,%.4f,%.4f,%d,%.4f,%.4f,%d,%d,%.4f,%d,%s,%.4f,%d,%d,'%s','%s','%s',\
					%d,%d,%d,%.4f,%d,%d,%d,%.4f, %d,'%s',%.4f,'%s','%s',%.4f,'%s')",//d to f 2013-2-27 15:30:11 proj2013-1-23-1 称重商品 加入 会员促销、限购
					GetFormatDTString(m_stSaleDT), GetSalePos()->GetCasher()->GetUserID(),
					1, merchInfo->szPLU, merchInfo->szSimpleName,
					merchInfo->nManagementStyle, merchInfo->fSysPrice, merchInfo->fLabelPrice,
					merchInfo->fActualPrice, merchInfo->nSaleCount, 
					merchInfo->fSaleAmt,merchInfo->fSaleDiscount, 
					merchInfo->nMakeOrgNO, merchInfo->nMerchState, merchInfo->fZKPrice,
					merchInfo->nMerchID,
					GetFieldStringOrNULL(merchInfo->tSalesman.nHumanID, INVALIDHUMANID),
					merchInfo->fSaleQuantity,merchInfo->bFixedPriceAndQuantity ,iItem,	merchInfo->szPLUInput,merchInfo->szClsCode,merchInfo->szDeptCode,	merchInfo->nPromoID,merchInfo->nPromoStyle,
					merchInfo->LimitStyle,merchInfo->LimitQty,merchInfo->CheckedHistory,merchInfo->scantime,
					merchInfo->IncludeSKU, merchInfo->fActualBoxPrice,
					merchInfo->nItemType, merchInfo->strOrderNo, merchInfo->dCondPromDiscount,
					merchInfo->strArtID,merchInfo->strOrderItemID, merchInfo->fManualDiscount,
					merchInfo->strInvoiceEndNo);//aori add for limit sale syn 2010-12-24 9:44:56 加新字段 CheckHistory 同步 saleitem_temp
			}
			else
			{
				strSQL.Format("INSERT INTO SaleItem_Temp(SaleDT,HumanID,SaleID,PLU,SimpleName,"
					"ManagementStyle,NormalPrice,LablePrice,RetailPrice,SaleCount,"
					"SaleAmt,SaleDiscount,MakeOrgNO,MerchState,DiscountPrice,"
					"MerchID, SalesHumanID,SaleQuantity,ISFixedPriceAndQuantity,itemID,ScanPLU,ClsCode,DeptCode,promoID,PromoStyle "//thinpon ,IsDiscountLabel, BackDiscount)"
					",LimitStyle,LimitQty,CheckHistory,InputSec,IncludeSKU,BoxPrice,ItemType,OrderID,CondPromDiscount,ArtID,OrderItemID,ManualDiscount)"//aori add for limit sale syn 2010-12-24 9:44:56 加新字段 CheckHistory 同步 saleitem_temp
					"VALUES ('%s',%d,%d,'%s','%s',%d,%.4f,%.4f,%.4f,%d,%.4f,%.4f,%d,%d,%.4f,%d,%s,%.4f,%d,%d,'%s','%s','%s',\
					%d,%d,%d,%.4f,%d,%d,%d,%.4f, %d,'%s',%.4f,'%s','%s',%.4f)",//d to f 2013-2-27 15:30:11 proj2013-1-23-1 称重商品 加入 会员促销、限购
					GetFormatDTString(m_stSaleDT), GetSalePos()->GetCasher()->GetUserID(),
					1, merchInfo->szPLU, merchInfo->szSimpleName,
					merchInfo->nManagementStyle, merchInfo->fSysPrice, merchInfo->fLabelPrice,
					merchInfo->fActualPrice, merchInfo->nSaleCount, 
					merchInfo->fSaleAmt,merchInfo->fSaleDiscount, 
					merchInfo->nMakeOrgNO, merchInfo->nMerchState, merchInfo->fZKPrice,
					merchInfo->nMerchID,
					GetFieldStringOrNULL(merchInfo->tSalesman.nHumanID, INVALIDHUMANID),
					merchInfo->fSaleQuantity,merchInfo->bFixedPriceAndQuantity ,iItem,	merchInfo->szPLUInput,merchInfo->szClsCode,merchInfo->szDeptCode,	merchInfo->nPromoID,merchInfo->nPromoStyle,
					merchInfo->LimitStyle,merchInfo->LimitQty,merchInfo->CheckedHistory,merchInfo->scantime,
					merchInfo->IncludeSKU, merchInfo->fActualBoxPrice,
					merchInfo->nItemType, merchInfo->strOrderNo, merchInfo->dCondPromDiscount,
					merchInfo->strArtID,merchInfo->strOrderItemID, merchInfo->fManualDiscount);//aori add for limit sale syn 2010-12-24 9:44:56 加新字段 CheckHistory 同步 saleitem_temp
			}
	    	
			iItem=iItem+1;
			pDB->Execute2(strSQL);//aori change 2011-3-22 16:51:36 优化clientdb ：删除execute2

			CUniTrace::Trace(strSQL);
		}
	}
	catch(CException* pe) 
	{
		GetSalePos()->GetWriteLog()->WriteLogError(*pe);
		pe->Delete();
		CDataBaseManager::closeConnection(pDB);
		CUniTrace::Trace(strSQL);
		return false;
	}
	CDataBaseManager::closeConnection(pDB);
	return true;
}
//======================================================================
// 函 数 名：UpdateItemInTemp
// 功能描述：更新零时销售数据
// 输入参数：int nSID
// 输出参数：bool
// 创建日期：00-2-21
// 修改日期：2009-4-1 曾海林
// 作  者：***
// 附加说明：数据库变更修改 SaleItem_Temp 
//======================================================================
bool CSaleImpl::UpdateItemInTemp(int nSID)
{//修改临时数据库中的数据
	if ( nSID >= m_vecSaleMerch.size() )
		return false;

	CPosClientDB* pDB = CDataBaseManager::getConnection(true);
	if ( !pDB ) {
		CUniTrace::Trace("CSaleImpl::UpdateItemInTemp");
		CUniTrace::Trace(MSG_CONNECTION_NULL, IT_ERROR);
		return false;
	}

	SaleMerchInfo* merchInfo = &m_vecSaleMerch[nSID];
	if(merchInfo == NULL)
	{
		CUniTrace::Trace("CSaleImpl::UpdateItemInTemp merchInfo is NULL");
		return false;
	}
	check_lock_saleitem_temp();//aori later 2011-8-7 8:38:11  proj2011-8-3-1 A 用 mutex等判断两个checkout之间 是否出现了update事件
	VERIFY(merchInfo);
	//CUniTrace::Trace("!!!将对SaleItem_Temp进行更新");//aori add 2011-8-3 17:03:07 proj2011-8-3-1 销售不平 条件促销 临时表 记日志里//2013-2-27 15:30:11 proj2013-1-23-1 称重商品 加入 会员促销、限购 
	
	CString strSQL;
	strSQL.Format("Update SaleItem_Temp set NormalPrice=%.4f,LablePrice=%.4f,RetailPrice=%.4f,SaleCount=%d, \
		SaleAmt=%.4f,SaleDiscount=%.4f,MakeOrgNO=%d,MerchState=%d,DiscountPrice=%.4f,MerchID=%d,SalesHumanID=%s,\
		LimitStyle=%d,LimitQty=%.4f ,CheckHistory=%d ,PromoID = %d \
		, PromoStyle = %d,CondPromDiscount=%.4f \
		,ManualDiscount = %.4f\
		,SaleQuantity = %.4f"//更新字段SaleQuantity，因为拆零商品允许的数量带小数点，输入后，需要更新[dandan.wu 2016-4-15]
		//aori add 2011-3-1 10:01:40 bug:限购style没有记录到表里
		//", SalesHumanID = %d "//aori add 2011-3-1 16:49:16 bug：限购的 makeOrgNO,PromoStyle,promid,SalesHumanID 没记到表里  
		
		"WHERE  HumanID = %d and ItemID = %d ", //SaleDT = '%s' AND
		merchInfo->fSysPrice, merchInfo->fLabelPrice,merchInfo->fActualPrice,
		merchInfo->nSaleCount,merchInfo->fSaleAmt,merchInfo->fSaleDiscount, 
		merchInfo->nMakeOrgNO, merchInfo->nMerchState, merchInfo->fZKPrice,
		merchInfo->nMerchID,
		GetFieldStringOrNULL(merchInfo->tSalesman.nHumanID, INVALIDHUMANID),
		merchInfo->LimitStyle,merchInfo->LimitQty,merchInfo->CheckedHistory,merchInfo->nPromoID,//aori add 2010-12-29 15:52:44 重启后 恢复限购信息  limit sale syn 2010-12-24 9:44:56 加新字段 CheckHistory 同步 saleitem_temp
		merchInfo->nPromoStyle,merchInfo->dCondPromDiscount,	//aori add 2011-3-1 10:01:40 bug:限购style没有记录到表里
		//merchInfo->nPromoID,//2011-3-1 16:49:16 bug：限购的 makeOrgNO,PromoStyle,promid,SalesHumanID 没记到表里 
		merchInfo->fManualDiscount,
		merchInfo->fSaleQuantity,//更新字段SaleQuantity，因为拆零商品允许的数量带小数点，输入后，需要更新[dandan.wu 2016-4-15]
		 GetSalePos()->GetCasher()->GetUserID(),nSID);//GetFormatDTString(m_stSaleDT),
	TRACE(strSQL);
	//CUniTrace::Trace("proj 2011-8-3-1 update saleitem_temp");//aori add proj 2011-8-3-1 销售不平 2011-10-27 11:33:33 补充日志
	pDB->Execute2(strSQL);//aori change 2011-3-22 16:51:36 优化clientdb ：删除execute2
	//AfxMessageBox((LPCSTR)strSQL, MB_ICONSTOP | MB_YESNO);
	CDataBaseManager::closeConnection(pDB);
	return true;
}

bool CSaleImpl::UpdateItemInTemp_scantime(int nSID,int p_scantime)
{//修改临时数据库中的数据
	if ( nSID >= m_vecSaleMerch.size() )
		return false;

	CPosClientDB* pDB = CDataBaseManager::getConnection(true);
	if ( !pDB ) {
		CUniTrace::Trace("CSaleImpl::UpdateItemInTemp_scantime");
		CUniTrace::Trace(MSG_CONNECTION_NULL, IT_ERROR);
		return false;
	}

	SaleMerchInfo* merchInfo = &m_vecSaleMerch[nSID];
	if(merchInfo == NULL)
	{
		CUniTrace::Trace("CSaleImpl::UpdateItemInTemp_scantime merchInfo is NULL");
		return false;
	}
	check_lock_saleitem_temp();
	VERIFY(merchInfo);
	CString strSQL;
	strSQL.Format("Update SaleItem_Temp set InputSec = %d "
		"WHERE  HumanID = %d and ItemID = %d ", 
		  p_scantime, GetSalePos()->GetCasher()->GetUserID(),nSID);//GetFormatDTString(m_stSaleDT),
	TRACE(strSQL);
	pDB->Execute2(strSQL);//aori change 2011-3-22 16:51:36 优化clientdb ：删除execute2
	CDataBaseManager::closeConnection(pDB);
	return true;
}


//##ModelId=3E641FAA034D
//删除其他用户的销售零时数据 2009-05-11
bool CSaleImpl::HasUncompletedSale()
{
	CPosClientDB* pDB = CDataBaseManager::getConnection(true);
	if ( !pDB ) {
		CUniTrace::Trace("CSaleImpl::HasUncompletedSale");
		CUniTrace::Trace(MSG_CONNECTION_NULL, IT_ERROR);
		return false;
	}

	CString strSQL;
	bool bHas = false;

	strSQL.Format("SELECT COUNT(*) FROM Sale_Temp WHERE HumanID = %d", 
		GetSalePos()->GetCasher()->GetUserID());
	CRecordset* pRecord = NULL;
	CDBVariant var;
	try {
		if ( pDB->Execute(strSQL, &pRecord) > 0 ) {
			pRecord->GetFieldValue((short)0, var);
			if ( var.m_lVal > 0 )
				bHas = true;
			pDB->CloseRecordset(pRecord);
		}
	} catch (CException* e) {
		CUniTrace::Trace("CSaleImpl::HasUncompletedSale");
		CUniTrace::Trace(*e);
		e->Delete();
	}

	
	try 
	{
		//删除其他用户的销售零时数据
		strSQL.Format("DELETE FROM Sale_Temp WHERE HumanID != %d", GetSalePos()->GetCasher()->GetUserID());
		pDB->Execute2(strSQL);//aori change 2011-3-22 16:51:36 优化clientdb ：删除execute2
		strSQL.Format("DELETE FROM SaleItem_Temp WHERE HumanID != %d", GetSalePos()->GetCasher()->GetUserID());
		check_lock_saleitem_temp();//aori later 2011-8-7 8:38:11  proj2011-8-3-1 A 用 mutex等判断两个checkout之间 是否出现了update事件
		pDB->Execute2(strSQL);//aori change 2011-3-22 16:51:36 优化clientdb ：删除execute2
	} catch (CException* e) 
	{
		pDB->CloseRecordset(pRecord);
		CUniTrace::Trace("CSaleImpl::LoadUncompleteSale");
		CUniTrace::Trace(*e);
		e->Delete();
	}
	

	CDataBaseManager::closeConnection(pDB);
	return bHas;
}

//======================================================================
// 函 数 名：LoadUncompleteSale
// 功能描述：装载未完成的销售
// 输入参数：int nSID
// 输出参数：bool
// 创建日期：00-2-21
// 修改日期：2009-4-1 曾海林
// 修改日期：2009-4-6 曾海林 修改打印部分
// 作  者：***
// 附加说明：数据库变更修改 SaleItem_Temp
//======================================================================
bool CSaleImpl::LoadUncompleteSale()
{
	CPosClientDB* pDB = CDataBaseManager::getConnection(true);
	if ( !pDB ) {
		CUniTrace::Trace("CSaleImpl::HasUncompletedSale");
		CUniTrace::Trace(MSG_CONNECTION_NULL, IT_ERROR);
		return false;
	}

	CString strSQL;
	CRecordset* pRecord = NULL;
	CDBVariant var;
	bool bHas = false;
	//会员平台增加输入会员数据
	strSQL.Format("SELECT SaleDT,MemberCode,CharCode,Shift_no ,EOrderID,EMemberID,EMOrderStatus,MemCardNO,MemCardNOType "
		          "  FROM Sale_Temp WHERE HumanID = %d", 
		GetSalePos()->GetCasher()->GetUserID());
	
	try {
		if ( pDB->Execute(strSQL, &pRecord) > 0 ) {
			pRecord->GetFieldValue((short)0, var);
			TStoST(*var.m_pdate, m_stSaleDT);

			pRecord->GetFieldValue(1, var);

			//非会员平台
			if (GetSalePos()->GetParams()->m_nUseRemoteMemScore != VIPCARD_QUERYMODE_MEMBERPLATFORM)
			{
				if(var.m_dwType != DBVT_NULL && strlen(*var.m_pstring)>0)
				{
					//m_MemberImpl.SetMember(*var.m_pstring);
					//if(SetMemberCode(*var.m_pstring,0,0,1)==1)//if(SetMemberCode(*var.m_pstring,0,0,1))aori change bug:原返回-1时认为成功 2011-3-28 16:29:52 setmembercode 优化
					m_MemberImpl.QueryMember(*var.m_pstring,VIPCARD_NOTYPE_FACENO);
				}
			}
			pRecord->GetFieldValue(2, var);
			if ( var.m_dwType != DBVT_NULL ) 
				m_strCustomerCharacter = *var.m_pstring;
			else
				m_strCustomerCharacter = "";

			//thinpon 获取未完成销售班次.
			pRecord->GetFieldValue(3, var);
			m_uShiftNo=atof(*var.m_pstring);
			//获取电商订单号
			pRecord->GetFieldValue(4,var);
			if(var.m_dwType != DBVT_NULL && strlen(*var.m_pstring)>0) 
				m_EorderID = *var.m_pstring;
			else 
				m_EorderID = "";
			//获取电商会员号
			pRecord->GetFieldValue(5,var);
			if(var.m_dwType != DBVT_NULL && strlen(*var.m_pstring)>0)
				m_EMemberID = *var.m_pstring;
			else 
				m_EMemberID = "";
			//获取电商订单状态
			pRecord->GetFieldValue(6,var);
			if(var.m_dwType != DBVT_NULL)
				m_EMOrderStatus =var.m_chVal;
			else 
				m_EMOrderStatus = 0;

			//会员平台增加项
			CString m_szMemCardNO;
			pRecord->GetFieldValue(7,var);
			if(var.m_dwType != DBVT_NULL)
				m_szMemCardNO = *var.m_pstring;
			else
				m_szMemCardNO = "";

			int MemCardNOType=0;
			pRecord->GetFieldValue(8,var);
			MemCardNOType = var.m_chVal;
			//会员平台
			if (GetSalePos()->GetParams()->m_nUseRemoteMemScore == VIPCARD_QUERYMODE_MEMBERPLATFORM)
			{
				if(MemCardNOType >0 && m_szMemCardNO.GetLength()>0)
				{
					//m_MemberImpl.SetMember(*var.m_pstring);
					//if(SetMemberCode(*var.m_pstring,0,0,1)==1)//if(SetMemberCode(*var.m_pstring,0,0,1))aori change bug:原返回-1时认为成功 2011-3-28 16:29:52 setmembercode 优化
					m_MemberImpl.QueryMember(m_szMemCardNO,MemCardNOType);
				}
			}
			pDB->CloseRecordset(pRecord);
			bHas = true;

			//thinpon.重打印未完成小票的表头.
			if (GetSalePos()->m_params.m_nPrinterType==2)
			PrintBillHeadSmall();
			
		}
	} catch (CException* e) {
		pDB->CloseRecordset(pRecord);
		CUniTrace::Trace("CSaleImpl::HasUncompletedSale");
		CUniTrace::Trace(*e);
		e->Delete();
	}	

strSQL.Format("SELECT a.SaleID, a.PLU, a.SimpleName, a.ManagementStyle, "
		"a.NormalPrice, a.LablePrice, a.RetailPrice, a.SaleCount, a.SaleAmt, "
		"a.SaleDiscount, a.MakeOrgNO, a.MerchState, a.DiscountPrice, "
		"a.MerchID, a.SalesHumanID, a.SaleQuantity,a.ISFixedPriceAndQuantity,"
		"isnull(a.PromoStyle,0),isnull(a.PromoID,0),isnull(a.OldSaleAmt,0),a.ScanPLU,"
		"isnull(a.MemberDiscount,0),a.ClsCode,a.DeptCode,isnull(a.ManualDiscount,0),b.merchtype, "
		"a.LimitStyle,isnull(b.LimitStyle,0),a.LimitQty,b.RetailPrice,a.CheckHistory "//aori add limit sale syn 2010-12-24 9:44:56 加新字段 CheckHistory 同步 saleitem_temp;//aori add	2011-1-5 8:32:58 加rowLimitStyle
		",isnull(b.AddCol1,0),"//2011-9-13 16:10:51 proj2011-9-13-2 印花 测试版在 62上崩溃//aori add proj2011-8-30-1.2 印花需求  2 对新字段的读写同步
		//<--#服务调用接口
		"isnull(SVID,0),isnull(SVNum,''),isnull(SVStatus,0) ,isnull(InputSec,0),isnull(a.IncludeSKU,1),isnull(a.BoxPrice,a.RetailPrice)" 
		//-->
		",a.ItemType,a.OrderID,a.OrderItemID,isnull(b.VipDiscount,0),b.ArtID"
		" FROM SaleItem_Temp a, salemerch b WHERE a.PLU=b.PLU and HumanID = %d order by ItemID",
		GetSalePos()->GetCasher()->GetUserID());CUniTrace::Trace(strSQL);
	pRecord = NULL;
	try {
		if ( pDB->Execute(strSQL, &pRecord) > 0 ) {
			for ( ; !pRecord->IsEOF(); pRecord->MoveNext() ) {
				
				pRecord->GetFieldValue((short)0, var);
				short nSID = var.m_iVal;//2013-2-4 15:48:20 proj2013-1-17 优化 saleitem 重启丢商品
				pRecord->GetFieldValue(1, var);
				SaleMerchInfo merchInfo(*var.m_pstring);
				merchInfo.nSID=nSID;//2013-2-4 15:48:20 proj2013-1-17 优化 saleitem 重启丢商品
				pRecord->GetFieldValue(2, var);
				merchInfo.szSimpleName= *var.m_pstring;

				pRecord->GetFieldValue(3, var);
				merchInfo.nManagementStyle = var.m_chVal;

				pRecord->GetFieldValue(4, var);
				merchInfo.fSysPrice = atof(*var.m_pstring);

				pRecord->GetFieldValue(5, var);
				merchInfo.fLabelPrice = atof(*var.m_pstring);
				merchInfo.fLabelPriceNORMAL = atof(*var.m_pstring);

				pRecord->GetFieldValue(6, var);
				merchInfo.fActualPrice = atof(*var.m_pstring);

				pRecord->GetFieldValue(7, var);
				merchInfo.nSaleCount = var.m_lVal;

				pRecord->GetFieldValue(8, var);
				merchInfo.fSaleAmt = merchInfo.fSaleAmt_BeforePromotion= atof(*var.m_pstring);//aori add 2012-2-7 16:11:04 proj 2.79 测试反馈：条件促销时 不找零 加 fSaleAmt_BeforePromotion

				pRecord->GetFieldValue(9, var);
				merchInfo.fSaleDiscount = atof(*var.m_pstring);

				pRecord->GetFieldValue(10, var);
				merchInfo.nMakeOrgNO = var.m_lVal;

				pRecord->GetFieldValue(11, var);
				merchInfo.nMerchState = var.m_chVal;

				pRecord->GetFieldValue(12, var);
				merchInfo.fZKPrice = atof(*var.m_pstring);

				
				pRecord->GetFieldValue(13, var);
				merchInfo.nMerchID = var.m_lVal;

				pRecord->GetFieldValue(14, var);
				merchInfo.nSimplePromoID = var.m_lVal;//简单促销号借用
				if(var.m_dwType != DBVT_NULL)
				{
					SetSalesMan(var.m_lVal);//营业员
					SetItemSalesMan(merchInfo);
				}
				else
					ClearCashier(m_tSalesman);

				pRecord->GetFieldValue(15, var);
				merchInfo.fSaleQuantity = atof(*var.m_pstring);
				
				pRecord->GetFieldValue(16, var);
				merchInfo.bFixedPriceAndQuantity = ( var.m_chVal == 1 );	
				

				pRecord->GetFieldValue(17, var);
				merchInfo.nPromoStyle = var.m_chVal;
				
				pRecord->GetFieldValue(18, var);
				merchInfo.nPromoID = var.m_lVal;
					
				/*if (bAssign&&(iPromoStyle==4||iPromoStyle==5))//有促销 //mabh
				{	pRecord->GetFieldValue(19, var);
					fSaleAtm_temp =atof(*var.m_pstring);
					merchInfo.fSaleAmt = fSaleAtm_temp;
				}*/
			
				pRecord->GetFieldValue(20, var);
				merchInfo.szPLUInput=*var.m_pstring;

				pRecord->GetFieldValue(21, var);
				merchInfo.fMemSaleDiscount = atof(*var.m_pstring);//记会员折扣
				merchInfo.fMemberShipPoint = atof(*var.m_pstring);//记红利积分借用

				pRecord->GetFieldValue(22, var);
				merchInfo.szClsCode= *var.m_pstring;

				pRecord->GetFieldValue(23, var);
				merchInfo.szDeptCode= *var.m_pstring;

				pRecord->GetFieldValue(24, var);
				merchInfo.fManualDiscount = atof(*var.m_pstring);//单品折扣
				merchInfo.fItemDiscount = atof(*var.m_pstring);//单品折扣借用

				//fManualDiscount<0:MarkUp,fManualDiscount>0:MarkDown [dandan.wu 2017-11-1]
				if(merchInfo.fManualDiscount != 0)
					merchInfo.nMerchState = RS_DS_MANUAL;
					
				//merchInfo.bIsDiscountLabel= merchInfo.nPromoStyle==3 ? 1:0;
				CString scanPlu=merchInfo.szPLUInput;
				merchInfo.bIsDiscountLabel=  merchInfo.nPromoStyle==3 ? 1:0 || scanPlu.GetLength()==m_pParam->m_nDiscountLabelBarLength;

				pRecord->GetFieldValue(25, var);//nMerchType
				merchInfo.nMerchType=var.m_chVal;
				//aori add limit sale later syn
				pRecord->GetFieldValue(26, var);
				merchInfo.LimitStyle= var.m_chVal;
				pRecord->GetFieldValue(27, var);
				merchInfo.rowLimitStyle= var.m_chVal;
				pRecord->GetFieldValue( 28, var );//aori add
				merchInfo.LimitQty= var.m_dwType != DBVT_NULL?atof(*var.m_pstring):0;
				pRecord->GetFieldValue(29, var);
				merchInfo.fRetailPrice = atof(*var.m_pstring);
				pRecord->GetFieldValue(30, var );//aori add 2010-12-24 9:44:56 加新字段 CheckHistory 同步 saleitem_temp
				merchInfo.CheckedHistory=(CHECKED_HISTORY) var.m_chVal;
				pRecord->GetFieldValue(31, var);merchInfo.StampRate=atof( *var.m_pstring);////aori add proj2011-8-30-1.2 印花需求  2 对新字段的读写同步
				IsLimitSale(&merchInfo);//aori add format salemerchinfo.blimitsale
				//thinpon 打印商品行 
				((CPosApp *)AfxGetApp())->GetSaleDlg()->m_dbInputCount=merchInfo.fSaleQuantity/*nSaleCount*/;//aori add 2011-11-29 12:41:23：proj 2011-11-4-1 T1472 支付智能识别 2011-11-26 8:28:14 测试反馈	pos扫描界面，重启后pos打印的商品数量和金额为零。
				((CPosApp *)AfxGetApp())->GetSaleDlg()->PrintSaleZhuHang(&merchInfo);//aori optimize 2011-10-13 14:53:19 逐行打印 del //if (GetSalePos()->m_params.m_nPrinterType==2)PrintBillItemSmall(merchInfo.szPLU,merchInfo.szSimpleName,merchInfo.nSaleCount,merchInfo.fActualPrice,merchInfo.fSaleAmt);
				// 重设扫描计时和支付计时
				secsScan = GetLocalTimeSecs();
				secsPay  = GetLocalTimeSecs();
				//<--#服务调用接口
				pRecord->GetFieldValue(32, var);//SVID
				unsigned char svID=var.m_chVal;
				if(merchInfo.nMerchState==RS_NORMAL && svID!=0)
				{
					if(m_ServiceImpl.SetMerchSV(m_vecSaleMerch))
					{
						pRecord->GetFieldValue(33, var);//SVNum
						char svNum[60];
						strcpy(svNum, *var.m_pstring);

						pRecord->GetFieldValue(34, var);//SVStatus
						unsigned char svStatus=var.m_chVal;

						m_ServiceImpl.SetSVNum(svNum);
						m_ServiceImpl.SetSVStatus(svStatus);
					}
				}
				//-->
				//扫描商品时间 mabh
				pRecord->GetFieldValue(35, var);//nMerchType
				merchInfo.scantime=atof(*var.m_pstring);
				//入数
				pRecord->GetFieldValue(36, var);
				merchInfo.IncludeSKU = var.m_lVal;
				//箱价
				pRecord->GetFieldValue(37, var);
				merchInfo.fActualBoxPrice = atof(*var.m_pstring);
				merchInfo.fBoxLabelPrice = merchInfo.fActualBoxPrice;

				//商品类型
				pRecord->GetFieldValue(38, var);
				merchInfo.nItemType = var.m_chVal;
				//订单号
				pRecord->GetFieldValue(39,var);
				merchInfo.strOrderNo = *var.m_pstring;
				//条件促销
				pRecord->GetFieldValue(40,var);
				merchInfo.strOrderItemID = var.m_dwType != DBVT_NULL?*var.m_pstring:"";;
				//是否参与会员折扣
				pRecord->GetFieldValue(41,var);
				merchInfo.bVipDiscount = var.m_chVal;
				pRecord->GetFieldValue(42,var);
				merchInfo.strArtID=var.m_dwType != DBVT_NULL?*var.m_pstring:"";
				m_vecSaleMerch.push_back(merchInfo);//aori later 2011-9-6 9:49:29 proj2011-8-30-1.1 印花需求 加读取salemerc表的StampRate（add1）字段
			}
			pDB->CloseRecordset(pRecord);
			bHas = true;
		}
	} catch (CException* e) {
		pDB->CloseRecordset(pRecord);
		CUniTrace::Trace("CSaleImpl::LoadUncompleteSale");
		CUniTrace::Trace(*e);
		e->Delete();
	}
	m_stratege_xiangou.ReGeneralLimitSaleInfosLeftCount();

	m_PayImpl.ReloadPaymentTemp(pDB, m_stSaleDT, m_nSaleID);//this->m_nSaleID);//aori change
	//礼券类别重新读取
	CCouponBNQImpl *CouponBNQImpl=GetSalePos()->GetCouponBNQImpl();
	if (CouponBNQImpl != NULL && CouponBNQImpl->m_bValid && m_PayImpl.m_vecPayItem.size() > 0)
	{
		for ( vector<PayInfoItem>::iterator payInfoItem = m_PayImpl.m_vecPayItem.begin(); payInfoItem != m_PayImpl.m_vecPayItem.end(); ++payInfoItem) 
		{
			if ( payInfoItem->nPSID == ::GetSalePos()->GetParams()->m_nCouponPSID ||
				payInfoItem->nPSID == ::GetSalePos()->GetParams()->m_nVirtualCouponPSID) 
			{
				CouponBNQImpl->GetCouponClassification(payInfoItem->szPaymentNum, payInfoItem->szExtraInfo);
			}
		}
	}
	//<--#服务调用接口
	if(m_PayImpl.m_vecPay.size()==0 && m_ServiceImpl.HasSVMerch())
		m_ServiceImpl.Clear();
	//-->
	GetSalePos()->GetPromotionInterface()->SumPromotionAmt(m_nSaleID);//this->m_nSaleID);//aori change

	//判断积分反馈，是否有已经成功提交，但支付未正常执行记录
	char shopperNo[20];
	CString czShopperNo="";
	char cshopperlog[50];
	
	GetSalePos()->GetPaymentInterface()->mobileLogFile->getItemValueByName("tomePass",shopperNo);
	czShopperNo.Format("%s",shopperNo);	
	if (GetSalePos()->GetPaymentInterface()->mobileLogFile->checkedReversal(CHECK_TYPE_ALL)){
		//积分回馈发现有未执行记录
		AfxMessageBox("手机支付异常，请完成后续操作。");
		
		sprintf(cshopperlog,"重启，发现有手机支付有未清除记录，shopperNo=%s。",shopperNo);
		GetSalePos()->GetWriteLog()->WriteLog(cshopperlog,1);
	}
	//判断积分反馈，是否有已经成功提交，但支付未正常执行记录——结束

	if(m_PayImpl.m_vecPay.size() > 0 || GetSalePos()->GetPromotionInterface()->m_fDisCountAmt >0 ||
		(m_vecSaleMerch.size()>0 &&GetSalePos()->GetPaymentInterface()->mobileLogFile->checkedReversal(CHECK_TYPE_ALL)))//检查手机充值是否需要进行冲正处理
	{
		m_IsLoadedUncompletedSale = true;
	}

	

	CDataBaseManager::closeConnection(pDB);	
	return bHas;
}

//##ModelId=3E641FAA0339
void CSaleImpl::EmptyTempSale()
{
	DelTempSaleDB();

/*
	CPosClientDB* pDB = CDataBaseManager::getConnection(true);
	if ( !pDB ) {
		CUniTrace::Trace("CSaleImpl::EmptyTempSale");
		CUniTrace::Trace(MSG_CONNECTION_NULL, IT_ERROR);
		return;
	}

	CString strSQL;
	int nCashierID = GetSalePos()->GetCasher()->GetUserID();

	strSQL.Format("DELETE FROM Sale_Temp WHERE HumanID = %d", nCashierID);
	pDB->Execute2(strSQL);//aori change 2011-3-22 16:51:36 优化clientdb ：删除execute2

	strSQL.Format("DELETE FROM SaleItem_Temp WHERE HumanID = %d", nCashierID);
	check_lock_saleitem_temp();//aori later 2011-8-7 8:38:11  proj2011-8-3-1 A 用 mutex等判断两个checkout之间 是否出现了update事件
	pDB->Execute2(strSQL);//aori change 2011-3-22 16:51:36 优化clientdb ：删除execute2

	m_PayImpl.DelAllTempPayment(pDB);

	CDataBaseManager::closeConnection(pDB);*/
}

//======================================================================
// 函 数 名：ReloadSale
// 功能描述：
// 输入参数：CPosClientDB *pDB, int nSaleID, SYSTEMTIME& stSaleDT
// 输出参数：bool
// 创建日期：00-2-21
// 修改日期：2009-4-1 曾海林
// 修改日期：2009-4-6 曾海林 修改打印部分
// 2009-4-5 修改打印
// 作  者：***
// 附加说明：数据库变更修改 saleitem,
//======================================================================
bool CSaleImpl::ReloadSale( int nSaleID, SYSTEMTIME& stSaleDT,bool pCheckLimitSale)
{	
	CAutoDB db(true);
	CString strSql;
	CString strTableDTExt;
	
	strTableDTExt.Format("%04d%02d", stSaleDT.wYear, stSaleDT.wMonth);
	CString strSpanBegin= GetDaySpanString(&stSaleDT);
	CString strSpanEnd= GetDaySpanString(&stSaleDT, false);
	
	int col = 0;//读取销售主表信息
	
	strSql.Format( "SELECT SaleDT, EndSaleDT, CheckSaleDT, "
		"MemberCode, CharCode, ManualDiscount, MemDiscount,Shift_No "
		"FROM Sale%s WHERE SaleID = %ld "
				" AND SaleDT >= '%s' AND SaleDT < '%s'",
				strTableDTExt, nSaleID, strSpanBegin, strSpanEnd);

	if ( db.Execute(strSql) > 0 ) 
	{
		TStoST(*db.GetFieldValue( col++).m_pdate, m_stSaleDT);

		CTime tmSaleDT(m_stSaleDT);

		strSql.Format("UPDATE Sale_Temp SET SaleDT='%s' WHERE HumanID = %d  ", 
			GetFormatDTString(m_stSaleDT), GetSalePos()->GetCasher()->GetUserID());
		db.Execute(strSql);//aori bug ??? !!! 2011-1-2 21:55:30 reloadsale时更新一个不存在的记录//aori change 2011-3-22 16:51:36 优化clientdb ：删除execute2
		
		CTimeSpan tmSpan(0, 0, 0, db.GetFieldValue( col++).m_lVal);

		CTime tmEndSaleDT = tmSaleDT;
		tmEndSaleDT += tmSpan;
		tmEndSaleDT.GetAsSystemTime(m_stEndSaleDT);

		CTimeSpan tmSpan2(0, 0, 0, db.GetFieldValue( col++).m_lVal);
		CTime tmCheckSaleDT = tmSaleDT;
		tmCheckSaleDT += tmSpan2;
		tmEndSaleDT.GetAsSystemTime(m_stCheckSaleDT);
		
		if( db.GetFieldValue( col++).m_dwType != DBVT_NULL && strlen(*db.m_var.m_pstring)>0 )
		{
			m_MemberImpl.QueryMember(*db.m_var.m_pstring,VIPCARD_NOTYPE_FACENO);
			//SetMemberCode(*var.m_pstring,0,0,1);//aori add 2010-12-30 21:36:47 bug 先解挂 再重启时 商品不显示限购 ，丢失了会员身份， 没有恢复 m_veclimitsaleinfo	
		}
		if(db.GetFieldValue( col++).m_dwType != DBVT_NULL)
		{
			m_strCustomerCharacter = (const char *)*db.m_var.m_pstring;
		}
		else 
		{	
			m_strCustomerCharacter = "";
		}

		m_fDiscount = atof(*db.GetFieldValue( col++).m_pstring);
		m_nDiscountStyle = m_fDiscount == 0.0?SDS_NO:SDS_PRICE;

		db.GetFieldValue( col++);
		if ( m_MemberImpl.m_Member.nHumanID > 0 )
		{
			m_nMemDiscountStyle = SDS_PRICE;
			m_fMemDiscount = atof(*db.m_var.m_pstring);
			GetSalePos()->GetPromotionInterface()->m_nHotReducedScore = m_fMemDiscount;
		}

		// thinpon. 获取班次 
		m_uShiftNo = atof(*db.GetFieldValue( col++).m_pstring);

		// 打印表头
		if (GetSalePos()->m_params.m_nPrinterType==2)PrintBillHeadSmall();

		//(扫描计时)和(支付计时)设为当前
		secsPay =secsScan = GetLocalTimeSecs();			
	} 
	else 
	{
		return false;
	}
		
	// reloadsalepromotion mabh
	GetSalePos()->GetPromotionInterface()->reloadPromotion(nSaleID,stSaleDT);
		
	//读取销售明细，先全部取出来//(去除 ManualDiscount,zenghl 20090409)			/*strSql.Format( "SELECT PLU, RetailPrice, SaleCount, SaleQuantity, SaleAmt,  PromoStyle, "				"MakeOrgNO, PromoID, ManualDiscount, SalesHumanID, MemberDiscount FROM SaleItem%s WHERE SaleID = %d AND ""SaleDT >= '%s' AND SaleDT < '%s' ORDER BY ItemID", strTableDTExt, nSaleID, strSpanBegin, strSpanEnd);*/
	strSql.Format( "select a.PLU, a.RetailPrice,b.RetailPrice,"		//aori add
		" a.SaleCount, a.SaleQuantity, a.SaleAmt,  a.PromoStyle,"	//aori change saleamt to addcol1 2012-2-10 10:18:18 proj 2.79 测试反馈：条件促销时 应付错误：未能保存、恢复商品的促销前saleAmt信息。db增加字段保存该信息
		"a.MakeOrgNO, a.PromoID,a.NormalPrice, a.SalesHumanID,"
		"isnull(a.MemberDiscount,0) , isnull(b.clscode,NULL),"
		"isnull(b.deptcode,NULL),isnull(a.ScanPLU,NULL),b.merchtype "
		",isnull(a.AddCol1,0) "	//aori add 2012-2-10 10:18:18 proj 2.79 测试反馈：条件促销时 应付错误：未能保存、恢复商品的促销前saleAmt信息。db增加字段保存该信息
		",isnull(a.ManualDiscount,0) "//aori add 不需addcol1了 2012-2-10 10:18:18 proj 2.79 测试反馈：条件促销时 应付错误：未能保存、恢复商品的促销前saleAmt信息。db增加字段保存该信息		
		",a.ItemType,a.OrderID,a.CondPromDiscount,a.ArtID,a.OrderItemID,a.MemDiscAfterProm" //Bnq新添加的字段 [dandan.wu 2016-1-28]
		",isnull(a.AddCol2,''),isnull(a.AddCol3,''),isnull(a.AddCol4,'') "//<--#服务调用接口
		"FROM SaleItem%s a,Salemerch b  "
			" WHERE a.plu= b.plu and a.SaleID = %d AND "
				" a.SaleDT >= '%s' AND a.SaleDT < '%s'  ORDER BY  a.ItemID ", 
				strTableDTExt, nSaleID, strSpanBegin, strSpanEnd);
	CUniTrace::Trace(strSql);
	if ( db.Execute(strSql) > 0 )//TRowSaleMerch saleMerch;//aori del//CSaleItem saleItem;
	{
		int nSID = 0;
		while( !db.m_pRecord->IsEOF() )
		{
			col = 0;
			
			SaleMerchInfo merchInfo( (const char *)*db.GetFieldValue( col++).m_pstring);//CSaleItem saleItem( merchInfo.szPLU );saleItem.FormatSaleMerchInfo(merchInfo);
		
			merchInfo.nSID = nSID++;
			
			merchInfo.fActualPrice = atof(*db.GetFieldValue( col++).m_pstring);//RetailPrice
			merchInfo.fRetailPrice= atof(*db.GetFieldValue( col++).m_pstring);//SaleCount
			merchInfo.nSaleCount = db.GetFieldValue( col++).m_lVal;//SaleCount
			merchInfo.fSaleQuantity = atof(*db.GetFieldValue( col++).m_pstring);//SaleQuantity
			merchInfo.fSaleAmt = atof(*db.GetFieldValue( col++).m_pstring);//aori del = merchInfo.fSaleAmt_BeforePromotion=2012-2-10 10:18:18 proj 2.79 测试反馈：条件促销时 应付错误：未能保存、恢复商品的促销前saleAmt信息。db增加字段保存该信息//SaleAmt//aori add 2012-2-7 16:11:04 proj 2.79 测试反馈：条件促销时 不找零 加 fSaleAmt_BeforePromotion
			merchInfo.nPromoStyle =	merchInfo.nDiscountStyle = db.GetFieldValue( col++).m_chVal;
			merchInfo.nMakeOrgNO = db.GetFieldValue( col++).m_lVal;//MakeOrgNO
			merchInfo.nPromoID =	merchInfo.nSalePromoID = db.GetFieldValue( col++).m_lVal;
			merchInfo.fSysPrice = atof(*db.GetFieldValue( col++).m_pstring); 
			merchInfo.fLabelPrice = merchInfo.fActualPrice;
			if(db.GetFieldValue( col++).m_dwType != DBVT_NULL)
			{
				SetSalesMan(db.m_var.m_lVal);
				SetItemSalesMan(merchInfo);
				merchInfo.nSimplePromoID=db.m_var.m_lVal;
			}
			else
			{
				ClearCashier(m_tSalesman);
			}
			merchInfo.fMemSaleDiscount = atof(*db.GetFieldValue( col++).m_pstring);//MemberDiscount
			merchInfo.szClsCode= *db.GetFieldValue( col++).m_pstring ;//clscode
			merchInfo.szDeptCode= *db.GetFieldValue( col++).m_pstring ;//deptcode
			merchInfo.szPLUInput= (const char *)*db.GetFieldValue( col++).m_pstring ;//isnull(a.ScanPLU,0)//szPLUInput//判断是否包含金额和是否称重商品//&bPriceInBarcode, &bFixedPriceAndQuantity//bool  bFixedPriceAndQuantity=false,bPriceInBarcode=false;//aori  2011-8-15 10:29:27 proj2011-8-15-1 称重商品小票不打印单价。
		
			merchInfo.m_saleItem.IsMoneyMerchCode(merchInfo.szPLUInput,&merchInfo.bPriceInBarcode,&merchInfo.bFixedPriceAndQuantity);
		
			merchInfo.bIsDiscountLabel =  merchInfo.nDiscountStyle==3 ? 1:0 || strlen(merchInfo.szPLUInput)==m_pParam->m_nDiscountLabelBarLength;	 
			
			merchInfo.nMerchType = db.GetFieldValue( col++).m_chVal;//nMerchType

			//merchInfo.fSaleAmt_BeforePromotion直接取AddCol1 [dandan.wu 2016-5-23]
			merchInfo.fSaleAmt_BeforePromotion = atof(*db.GetFieldValue( col++).m_pstring);//aori add 2012-2-10 10:18:18 proj 2.79 测试反馈：条件促销时 应付错误：未能保存、恢复商品的促销前saleAmt信息。db增加字段保存该信息
			 
			//获得手工折扣 [dandan.wu 2016-5-23]
			//merchInfo.fSaleAmt_BeforePromotion = atof(*db.GetFieldValue( col++).m_pstring)+merchInfo.fSaleAmt;//aori add 不需要addcol1了 2012-2-10 10:18:18 proj 2.79 测试反馈：条件促销时 应付错误：未能保存、恢复商品的促销前saleAmt信息。db增加字段保存该信息
			merchInfo.fManualDiscount = (atof)(*db.GetFieldValue( col++).m_pstring);
		
			//商品类型和订单号是BNQ新添加的字段，需要赋值，以便重打印时区分现货和订单 [Added by dandan.wu 2016-1-28]
			merchInfo.nItemType = db.GetFieldValue( col++).m_chVal;
			merchInfo.strOrderNo = *db.GetFieldValue( col++).m_pstring;
			merchInfo.dCondPromDiscount = (atof)(*db.GetFieldValue( col++).m_pstring);
			merchInfo.strArtID = *db.GetFieldValue( col++).m_pstring;
			merchInfo.strOrderItemID = *db.GetFieldValue( col++).m_pstring;
			merchInfo.dMemDiscAfterProm = (atof)(*db.GetFieldValue( col++).m_pstring);

			IsLimitSale( &merchInfo);//aori add 2010-12-29 16:53:52 限购功能不支持挂起、解挂  
			m_vecSaleMerch.push_back(merchInfo);//aori later 2011-9-6 9:49:29 proj2011-8-30-1.1 印花需求 加读取salemerc表的StampRate（add1）字段
			
			((CPosApp *)AfxGetApp())->GetSaleDlg()->PrintSaleZhuHang(&merchInfo);//aori optimize 2011-10-13 14:53:19 逐行打印 del //if (GetSalePos()->m_params.m_nPrinterType==2)PrintBillItemSmall(merchInfo.szPLU,merchInfo.szSimpleName,merchInfo.nSaleCount,merchInfo.fActualPrice,merchInfo.fSaleAmt);
			
			//<--#服务调用接口
			CString strSVID=*db.GetFieldValue( col++).m_pstring;
			if(!strSVID.IsEmpty())
			{
				unsigned char svID=atoi(strSVID);
				if(svID!=0)
				{
					if(m_ServiceImpl.SetMerchSV(m_vecSaleMerch))
					{
						char svNum[60];
						strcpy(svNum, *db.GetFieldValue( col++).m_pstring);
						m_ServiceImpl.SetSVNum(svNum);
						m_ServiceImpl.SetSVStatus(SV_STATUS_COMMIT_SUCCESS);
					}
				}
			}

			db.m_pRecord->MoveNext();
		}
		
		if(pCheckLimitSale)
		{
			m_stratege_xiangou.checkXianGouV2(TRIGGER_TYPE_NoCheck,-1,false);//aori add bQueryBuyingUpLimit 2011-4-26 9:44:43 限购问题：取消商品时总是询问是否购买超限商品
		}	
	}//end if ( pDB->Execute(strSql, &pRecord) > 0 )

	//设置当前的流水号为加载的流水号
	m_nSaleID = nSaleID;
	
	return true;
}

//##ModelId=3E641FAA0311
bool CSaleImpl::ReprintSale(int nSaleID, SYSTEMTIME& stSaleDT,int reprttype)
{
	//aori 2011-12-30 14:42:38 proj 2011-12-22-1 combine 2.75 with 2.71 saleimpl
	CPosClientDB *pDB = CDataBaseManager::getConnection(true);
	if ( !pDB ) 
		return false;

	Clear();//int bk_saleid=m_nSaleID;//aori add 2012-2-9 13:49:53 proj 2.79 测试反馈：重打印小票 应付金额错误
	
	m_nSaleID = nSaleID;//aori add 2011-10-14 10:54:21 bug 重打印时  单号错误

	CString strTableDTExt;
	strTableDTExt.Format("%04d%02d", stSaleDT.wYear, stSaleDT.wMonth);//aori 2011-12-30 14:42:38 proj 2011-12-22-1 combine 2.75 with 2.71 saleimpl
	
	bool bRet = false;
	if ( ReloadSale(nSaleID, stSaleDT,false) ) 
	{
		if ( m_PayImpl.ReloadPayment(pDB, m_stSaleDT, m_nSaleID) ) 
		{
			//税控调用//aori begin 2011-12-30 14:42:38 proj 2011-12-22-1 combine 2.75 with 2.71 saleimpl
			if (GetSalePos()->GetParams()->m_nPrintTaxInfo == 1)
			{
				GetSalePos()->GetWriteLog()->WriteLog("开始税控发票调用",3);//写日志
				CTaxInfoImpl *TaxInfoImpl=GetSalePos()->GetTaxInfoImpl();
				TaxInfoImpl->SetSaleImpl(this);
				TaxInfoImpl->SetPayImpl(&m_PayImpl);
				TaxInfoImpl->RePrtSaleTaxInfo(reprttype, m_nSaleID, strTableDTExt);
				if(reprttype == 1  || reprttype == 0)
				{
					//重打印发票操作记录
					CString strBuf;
					strBuf.Format("%d", m_nSaleID);
					m_pADB->SetOperateRecord(OPT_REPRTTAX, GetSalePos()->GetCasher()->GetUserID(), strBuf);
				}
				GetSalePos()->GetWriteLog()->WriteLog("结束税控发票调用",3);//写日志
			}//aori end 2011-12-30 14:42:38 proj 2011-12-22-1 combine 2.75 with 2.71 saleimpl

			m_bReprint = true;
			if (GetSalePos()->GetParams()->m_nBillPrintCount_Sale)
			{
				int nCount = GetSalePos()->GetParams()->m_nBillPrintCount_Sale;
				for (int i = 0; i<nCount; i++)
				{
					if(m_ServiceImpl.HasSVMerch())
						GetSalePos()->GetPrintInterface()->nSetPrintBusiness(PBID_SERVICE_RECEIPT);
					else
						GetSalePos()->GetPrintInterface()->nSetPrintBusiness(PBID_TRANS_RECEIPT);
					
					PrintBillLine(g_STRREPRINT);

					PrintBillWHole();
				}
			}
			bRet = true;
			m_bReprint = false;
		}

		CString strBuf;
		strBuf.Format("%d", nSaleID);
		CAccessDataBase *pADB = &GetSalePos()->m_pADB;
		pADB->SetOperateRecord(OPT_REPRINTBILL, GetSalePos()->GetCasher()->GetUserID(), strBuf);

		SYSTEMTIME stNow;
		GetLocalTimeWithSmallTime(&stNow);
		GetSalePos()->m_Task.putOperateRecord(stNow);
	}

	Clear();
	CDataBaseManager::closeConnection(pDB);

	//m_nSaleID=bk_saleid;//aori add	2012-2-9 13:49:53 proj 2.79 测试反馈：重打印小票 应付金额错误
	return bRet;
}

void CSaleImpl::PrintBillLine(const char* strbuf,int nAlign)
{	//aori add 2011-12-30 14:42:38 proj 2011-12-22-1 combine 2.75 with 2.71 saleimpl
	if (!GetSalePos()->GetPrintInterface()->m_bValid) 
	{
		//打印机不可用则退出
		GetSalePos()->GetWriteLog()->WriteLog("END PrintBillWhole PrintInterface::m_bValid禁止打印",4,POSLOG_LEVEL1);//aori add 2011-8-30 10:23:53 proj2011-8-30-2 优化可读性
		return;
	}

	int MaxLen = GetSalePos()->GetParams()->m_nMaxPrintLineLen;
	if (GetSalePos()->GetParams()->m_nPaperType == 1 )  
		MaxLen=30;
	GetPrintFormatAlignString(strbuf, m_szBuff, MaxLen, nAlign/*DT_CENTER*/);
	GetSalePos()->GetPrintInterface()->nPrintLine(m_szBuff);
}

bool CSaleImpl::GetItemSaleAmt(int nSID,double *SaleAmt)
{
	if ( nSID >= m_vecSaleMerch.size() ) 
		return false;
	SaleMerchInfo *merchInfo = &m_vecSaleMerch[nSID];
	if ( merchInfo->fSaleQuantity/*nSaleCount*/ == 0 )
		return false;
	if(merchInfo->nItemType == ITEM_IN_ORDER)
	{
		CString strOrderNO = m_vecSaleMerch[nSID].strOrderNo;
		for (int n = 0 ; n < (int)m_vecSaleMerch.size(); n++) 
		{
			if(m_vecSaleMerch[n].nItemType == ITEM_IN_ORDER && m_vecSaleMerch[n].strOrderNo == strOrderNO)
				*SaleAmt += merchInfo->fSaleAmt;	
		}
	}
	else
		*SaleAmt = merchInfo->fSaleAmt;
	return true;
}

//======================================================================待
// 函 数 名：ReCalculatePromoPrice()
// 功能描述：
// 输入参数：
// 输出参数：bool
// 创建日期：00-2-21
// 修改日期：2009-4-2 曾海林
// 作  者：***
// 附加说明：促销变更修改,需要重新整理
//======================================================================/
void CSaleImpl::ReCalculatePromoPrice()
{
	for ( vector<SaleMerchInfo>::iterator merchInfo = m_vecSaleMerch.begin(); merchInfo != m_vecSaleMerch.end(); ++merchInfo ) {
		if ( (merchInfo->nMerchState != RS_CANCEL) && !(merchInfo->bIsDiscountLabel))
		{
			//m_promotion.ReGetPromotionPrice(*merchInfo, GetMemberLevel());
			//MergeItems(merchInfo->szPLU);
		}
						
	}
	

}

//======================================================================待
// 函 数 名：ReCalculatePromoPrice()
// 功能描述：
// 输入参数：
// 输出参数：bool
// 创建日期：00-2-21
// 修改日期：2009-4-2 thinpon
// 修改日期：2009-5-18 ZENGHL 修改 //对变价商品不做处理
// 修改日期：2009523 ZENGHL 修改  防止重刷会员卡 18码出现金额三位小数
// 作  者：***
// 附加说明：会员变更修改,需要重新整理
//======================================================================/
void CSaleImpl::ReCalculateMemPrice(bool bNeedZeroLimitInfo){
	m_stratege_xiangou.m_vecBeforReMem=m_vecSaleMerch;//aori add 2011-5-11 9:14:43 proj2011-5-3-1:逐行打印完善：重输会员号、 取消商品 4 ：逐打被取消商品时，价格不对
	bNeedZeroLimitInfo?m_stratege_xiangou.m_VectLimitInfo.clear():1;//aori add 2011-3-3 10:39:30限购bug:连续输入不同会员号时价格错误 //aori add bNeedZeroLimitInfo 2011-4-8 14:33:28 limit bug 取消商品后 限购错误//int i=0;//aori del
	for ( vector<SaleMerchInfo>::iterator merchInfo = m_vecSaleMerch.begin(); merchInfo != m_vecSaleMerch.end(); ++merchInfo )
	{
		//fManualDiscount>0:MarkDown,fManualDiscount<0:MarkUp [dandan.wu 2017-11-1]
		if(merchInfo->m_saleItem.CanChangePrice() 
			|| merchInfo->fManualDiscount != 0 
			|| merchInfo->nItemType == ITEM_IN_ORDER)//订单商品不修改价格
			continue; //可修改价格的商品(大码)不刷新价格 markdwon后的商品不刷新价格
		
		merchInfo->bLimitSale=0;//aori add  2011-3-4 17:30:02 全体小票商品 bug 当会员价==折扣价时 会员购买超限全体小票时 应取 normal价
		SaleMerchInfo tmpmerch=*merchInfo;//aori add 2011-5-3 11:36:57 proj2011-5-3-1:逐行打印完善：重输会员号、 取消商品
		
		if ( (merchInfo->nMerchState != RS_CANCEL) && !(merchInfo->bIsDiscountLabel))
		{
			SaleMerchInfo ReCalmerchInfo(merchInfo->szPLUInput,GetMember().szMemberCode);
			ReCalmerchInfo.szPLUInput= merchInfo->szPLUInput;
            bool bFixedPriceAndQuantity=false,bPriceInBarcode=false;
			ReCalmerchInfo.m_saleItem.IsMoneyMerchCode(merchInfo->szPLUInput, &bPriceInBarcode, &bFixedPriceAndQuantity); 
  			merchInfo->nPromoStyle	=		ReCalmerchInfo.nPromoStyle	;
  			merchInfo->nMakeOrgNO	=  		ReCalmerchInfo.nMakeOrgNO	;	
  			merchInfo->nPromoID		=  		ReCalmerchInfo.nPromoID		;
			
			//aori add 2011-3-2 9:37:11 bug：第二次输入会员号时 价格错误
			merchInfo->CheckedHistory=ReCalmerchInfo.CheckedHistory;
			merchInfo->LimitStyle=ReCalmerchInfo.LimitStyle;
			merchInfo->fLabelPrice=ReCalmerchInfo.fLabelPrice;//2013-2-5 13:52:32 proj2013-1-17-5 优化 saleitem 先称后卡label价错
			if (ReCalmerchInfo.fActualPrice >0)
			{//&&ReCalmerchInfo.nManagementStyle!=MS_JE){ //大于0才作if (ReCalmerchInfo.nManagementStyle==MS_JE)//&&(ReCalmerchInfo.nMerchType==3||ReCalmerchInfo.nMerchType==6)){i++;continue;//对金额类,大码商品不作变价处理}
				merchInfo->fActualPrice= ReCalmerchInfo.fActualPrice;
				merchInfo->fSaleAmt=bFixedPriceAndQuantity?merchInfo->fSaleAmt:ReCalmerchInfo.fActualPrice * merchInfo->fSaleQuantity/*nSaleCount*/;
				
			}
		}//	i++;	//aori del
	}//bNeedReprint?ReprintAll():1;//aori add 5/10/2011 9:18:37 AM proj2011-5-3-1:逐行打印完善：重输会员号、 取消商品2
}

double CSaleImpl::GetItemDiscount()
{
	double fTotItemDiscount = 0.0f, fTemp = 0.0f;
	for (vector<SaleMerchInfo>::iterator merchInfo = m_vecSaleMerch.begin(); merchInfo != m_vecSaleMerch.end(); ++merchInfo ) 
	{
		fTemp = merchInfo->fSaleDiscount;
		fTotItemDiscount += ReducePrecision(fTemp);
	}
	ReducePrecision(fTotItemDiscount);
	return fTotItemDiscount;
}

bool CSaleImpl::SetSalesMan(const CString &SalesManCode)
{
	ClearCashier(m_tSalesman);
	//如果没有表示清空业务员
	if(SalesManCode.IsEmpty())
		return true;

	TCashier tCashierTemp;
	ClearCashier(tCashierTemp);
	strcpy(tCashierTemp.szUserCode, SalesManCode);
	CAccessDataBase* pADB = &GetSalePos()->m_pADB;

	if(pADB->GetCashier(tCashierTemp, GetSalePos()->IsSalerUsed(), GetSalePos()->GetParams()->m_nOrgRoleID))
	{
		memcpy(&m_tSalesman, &tCashierTemp, sizeof(TCashier));
	} 
	else 
	{
		return false;
	}

	return true;
}

bool CSaleImpl::SetSalesMan(int SalesHumanID)
{
	ClearCashier(m_tSalesman);
	//如果没有表示清空业务员
	if(SalesHumanID <= INVALIDHUMANID)
		return true;

	TCashier tCashierTemp;
	ClearCashier(tCashierTemp);
	tCashierTemp.nHumanID = SalesHumanID;
	CAccessDataBase* pADB = &GetSalePos()->m_pADB;

	if(pADB->GetCashier(tCashierTemp, GetSalePos()->IsSalerUsed(), GetSalePos()->GetParams()->m_nOrgRoleID))
	{
		memcpy(&m_tSalesman, &tCashierTemp, sizeof(TCashier));
	} 
	else 
	{
		return false;
	}

	return true;
}

//设定指定序号的销售的业务员
bool CSaleImpl::SetItemSalesMan(int nSID)
{
	if ( nSID < m_vecSaleMerch.size() ) 
	{
		if(IsValidPLUWithSalesHumanID(m_vecSaleMerch[nSID].szPLU, m_tSalesman.nHumanID, nSID))
			memcpy(&(m_vecSaleMerch[nSID].tSalesman), &m_tSalesman, sizeof(TCashier));
		else
			return false;

	}
	return true;
}

bool CSaleImpl::SetItemSalesMan(SaleMerchInfo &merchInfo)
{
	if(IsValidPLUWithSalesHumanID(merchInfo.szPLU, m_tSalesman.nHumanID))
		memcpy(&(merchInfo.tSalesman), &m_tSalesman, sizeof(TCashier));
	else
		return false;
	return true;
}
//判断业务员和商品的对应是否合法,由于汇总的原因同一个商品只能对应一个业务员
//判断要排除nSID对应的商品项
bool CSaleImpl::IsValidPLUWithSalesHumanID(const char* szPLU, int nSalesHumanID, int nSID)
{
	if(BCANSAMESALESMAN)
		return true;
	for ( vector<SaleMerchInfo>::iterator merchInfo = m_vecSaleMerch.begin(); merchInfo != m_vecSaleMerch.end(); ++merchInfo ) 
	{
		if ( (merchInfo->nMerchState != RS_CANCEL) && strcmp(merchInfo->szPLU, szPLU) == 0)
		{
			if(nSID == merchInfo- m_vecSaleMerch.begin())continue;
			if(merchInfo->tSalesman.nHumanID != nSalesHumanID)		
			{
				CString strTemp;
				strTemp.Format("商品%s已被业务员%s销售,\n设定为业务员%s销售(是)\n在另一笔销售中销售(否)",merchInfo->szSimpleName,merchInfo->tSalesman.szUserName,merchInfo->tSalesman.szUserName);
				if(AfxMessageBox(strTemp, MB_ICONSTOP | MB_YESNO) == IDYES)//aori 2012-2-29 14:53:17 测试反馈  proj 2012-2-13 折扣标签校验 ：提示框应死循环			
					return SetSalesMan(merchInfo->tSalesman.nHumanID);

			}

			return (merchInfo->tSalesman.nHumanID == nSalesHumanID);
		}
						
	}
	return true;
}

bool CSaleImpl::CanChangeCount()
{
	return !m_vecSaleMerch.back().bFixedPriceAndQuantity;
	return !m_bFixedPriceAndQuantity;
}
//计算所有的商品的会员折扣，如果商品有促销就不折，否则可以折扣
bool CSaleImpl::GetMemWholeDiscount(double fMemDiscountRate, double* fMemWholeDiscount)
{
	*fMemWholeDiscount = 0;
	double fAmt = 0;
	//如果没有会员折扣，就不算了
	if(fMemDiscountRate >= 1)
		return false;
	for ( vector<SaleMerchInfo>::iterator merchInfo = m_vecSaleMerch.begin(); merchInfo != m_vecSaleMerch.end(); ++merchInfo ) 
	{
		//如果有折扣标签就不算了
		if ( (merchInfo->nMerchState == RS_NORMAL) && ( merchInfo->fSaleQuantity/*nSaleCount*/ != 0 ) && (!merchInfo->bIsDiscountLabel)) 
		{			
			fAmt = merchInfo->fSaleAmt * fMemDiscountRate;
			ReducePrecision(fAmt);
			*fMemWholeDiscount += (merchInfo->fSaleAmt - fAmt);
		}
	}
	return true;
}

bool SaleMerchInfo_Promo(const SaleMerchInfo& x, const SaleMerchInfo& y) 
{
	return (y.nMerchState == RS_NORMAL && ((x.nMerchState != RS_CANCEL && x.nMerchState != RS_NORMAL) || x.nMerchState == RS_CANCEL ))
		|| (x.nMerchState == RS_CANCEL && y.nMerchState != RS_CANCEL);
}


//按照先促销后非促销的顺序重新排列销售
void CSaleImpl::ReSortSaleMerch()
{
	//只有有会员折扣的才重新排序
	if(!IsHaveMemDiscount())
		return;
	sort((SaleMerchInfo*)m_vecSaleMerch.begin(), (SaleMerchInfo*)m_vecSaleMerch.end(), SaleMerchInfo_Promo);
}

bool CSaleImpl::IsHaveMemDiscount()
{
	return (fabs(m_fMemDiscount) > 0.001);
}


/*==============================================================*/
//获取班次号		[ Added by thinpon on Mar 6,2006 ]  成功/失败 : !0 / 0
int CSaleImpl::GetShiftNO()
{
	CString strSQL;
	//unsigned int ShiftNO;2013-2-16 14:59:39 proj2013-2-16-1 优化 下笔销售
	CString var;
	CRecordset* pRecord = NULL;
	CPosClientDB *pDB = CDataBaseManager::getConnection(true);
	if ( !pDB )
		return 0;
	
	try {
		strSQL.Format("SELECT Shift_No FROM SHIFT");
		//获取班次
		if ( pDB->Execute( strSQL, &pRecord ) > 0 ){
			pRecord->GetFieldValue( (short)0, var );
			m_uShiftNo=atof((LPCTSTR)var);
		}
		else{//无班次则从序号1开始
			strSQL.Format("DELETE dbo.Shift;INSERT INTO SHIFT (Shift_NO,Shift_date) values (1,getdate())");	//aori add 2011-6-22 14:36:05 proj2011-6-22 shift 表会重复
			if (pDB->Execute2( strSQL) == 0)//aori change 2011-3-22 16:51:36 优化clientdb ：删除execute2
				return 0;
			m_uShiftNo=1;
		} 
		
	} catch( CException * e ) {
		CUniTrace::Trace(*e);
		e->Delete( );
		CDataBaseManager::closeConnection(pDB);
		return 0;
	}
	catch(...)
	{
		GetSalePos()->GetWriteLog()->WriteLogError("获取shiftno报错",1);
		CDataBaseManager::closeConnection(pDB);
		return 0;
	}

	CDataBaseManager::closeConnection(pDB);
	return m_uShiftNo;
}


BOOL CSaleImpl::UpdateShiftNo()
{

	CString strSQL;
	ULONG shiftNO= GetShiftNO();
	CString var;
	CRecordset* pRecord = NULL;
	
	CPosClientDB *pDB = CDataBaseManager::getConnection(true);
	if ( !pDB )
		return false;
	
	if(shiftNO<999)
		shiftNO++;
	else
		shiftNO=1;

	try {
		strSQL.Format("UPDATE SHIFT set Shift_No = %u",shiftNO);
		pDB->Execute2( strSQL);//aori change 2011-3-22 16:51:36 优化clientdb ：删除execute2
		//if ( pDB->Execute2( strSQL) > 0 ){
			//pDB->Execute2("commit");
			//return true;
		//}

		//else{//无班次则从序号1开始
		//	strSQL.Format("INSERT INTO SHIFT (Shift_NO,Shift_date) values (1,getdate())");	
		//	if (pDB->Execute( strSQL) == 0)
		//		return true;
		//	ShiftNO=1;
		//}
	
	} catch( CException * e ) {
		CUniTrace::Trace(*e);
		e->Delete( );
		CDataBaseManager::closeConnection(pDB);
		return false;
	}

	CDataBaseManager::closeConnection(pDB);
	return true;
}

//计算商品销售总件数
double CSaleImpl::GetTotalCount()
{
	return GetTotalCount(&m_vecSaleMerch);
}
//计算商品销售总件数
double CSaleImpl::GetTotalCount(const vector<SaleMerchInfo> *pVecSaleMerch)
{
	double dTotal = 0;
	for ( vector<SaleMerchInfo>::const_iterator merchInfo = pVecSaleMerch->begin();
		merchInfo != pVecSaleMerch->end(); ++merchInfo )
		{			
			if ( merchInfo->nMerchState != RS_CANCEL )/*&& (!bWithoutPromotion
				|| (bWithoutPromotion && merchInfo->nMerchState == RS_NORMAL )))*/
				if(merchInfo->IncludeSKU>1)
					dTotal +=merchInfo->fSaleQuantity/*nSaleCount*//merchInfo->IncludeSKU;
				else
					dTotal += merchInfo->fSaleQuantity/*nSaleCount*/;
		}
	ReducePrecision(dTotal);
	return dTotal;
}


//thinpon 获取当前时间[秒数]
long CSaleImpl::GetLocalTimeSecs()
{
	time_t ltime;
	time( &ltime );
	return ltime;
}
//thinpon 打印换班小票
BOOL CSaleImpl::PrintShiftBill(int shift_no)
{
	//打印机不可用则退出
	
	if (!GetSalePos()->GetPrintInterface()->m_bValid) 
		return false;

	//int nMaxLen = GetSalePos()->GetParams()->m_nMaxPrintLineLen;
	int i = 0;	
//	if ( nMaxLen < 32 )
	//if ( GetSalePos()->GetParams()->m_nPaperType == 1 ) //mabh
	//	nMaxLen = 32;
	CPrintInterface  *printer = GetSalePos()->GetPrintInterface();
	//char *pt = m_szBuff;
	//读取pos收费数据
	CFinDataManage m_FinDataManage;

	vectcadshift.clear();
	vectpaycadshift.clear();
	m_FinDataManage.GetChargeAmtDaily(m_stSaleDT,GetSalePos()->GetParams()->m_nPOSID,1,vectcadshift,vectpaycadshift); //mabh
	//getcadamt(shift_no);  mabh
 	printer->nSetPrintBusiness(PBID_SHIFT_BILL);
	printer->FeedPaper_Head(PBID_SHIFT_BILL);
	//打印横线
	//GetPrintChars(m_szBuff, nMaxLen, nMaxLen, true);
	printer->nPrintHorLine();

	GetPrintFormatAlignString("**********换班记录小票**********",m_szBuff,DT_CENTER);
	printer->nPrintLine( m_szBuff );

	SYSTEMTIME m_stSaleDT;
	GetLocalTimeWithSmallTime(&m_stSaleDT);
	sprintf(m_szBuff,"机台号码:%3d   时间:%04d-%02d-%02d %02d:%02d",GetSalePos()->GetParams()->m_nPOSID,m_stSaleDT.wYear,m_stSaleDT.wMonth,m_stSaleDT.wDay,m_stSaleDT.wHour,m_stSaleDT.wMinute);
	printer->nPrintLine( m_szBuff );

	sprintf(m_szBuff,"班次:%03d    收银员:%s",m_uShiftNo,GetSalePos()->GetCasher()->GetUserCode());
	printer->nPrintLine( m_szBuff );

	//班次开始时间
	SYSTEMTIME _tm;
	GetShiftBeginTime(_tm);
	sprintf(m_szBuff,"上机时间:%04d-%02d-%02d %02d:%02d",_tm.wYear,_tm.wMonth,_tm.wDay,_tm.wHour,_tm.wMinute);
	printer->nPrintLine( m_szBuff );
	//班次结束时间
	::GetLocalTimeWithSmallTime(&_tm);
	sprintf(m_szBuff,"下机时间:%04d-%02d-%02d %02d:%02d",_tm.wYear,_tm.wMonth,_tm.wDay,_tm.wHour,_tm.wMinute);
	printer->nPrintLine( m_szBuff );
	//开始单号
	sprintf(m_szBuff,"开始单号:%d",GetFirstSaleID(m_uShiftNo));
	printer->nPrintLine( m_szBuff );
	//结束单号
	sprintf(m_szBuff,"结束单号:%d",GetLastSaleID(m_uShiftNo));
	printer->nPrintLine( m_szBuff );
	//单据张数
	sprintf(m_szBuff,"单据张数:%d",GetShiftTotalSale(m_uShiftNo));
	printer->nPrintLine( m_szBuff );
	//品项数
	sprintf(m_szBuff,"销售品项数:%d",GetShiftTotalSaleItem(m_uShiftNo));
	printer->nPrintLine( m_szBuff );

	//支付方式及合计金额[最多10种支付方式]
// 	int numRec=0;
// 	CString str1("抹零");
// 	CString str2("不找零");
//     CString strtmp("");
// 
// 	float ShiftTotal=0;
// 	float ShiftrealTotal=0;
// 	char *title[20];
// 	for (int ti=0;ti<20;ti++) title[ti]=(char*)malloc(20);
// 	float amt[20];
// 	numRec = GetShiftPaymentAmt(m_uShiftNo,(char**)title,amt,10,10);
// 	if ( numRec > 0 ){
// 		for (ti=0;ti<numRec;ti++){
// 			sprintf(m_szBuff,"%s:%.02f",title[ti],amt[ti]);
// 			printer->nPrintLine( m_szBuff );
// 			strtmp.Format("%s",title[ti]);
// 			if ( strtmp == str1 ) //mabh
// 				ShiftrealTotal += amt[ti];
// 			if ( strtmp == str2 ) 
// 				ShiftrealTotal += amt[ti];
// 
// 			ShiftTotal += amt[ti];
// 		}		
// 	}
// 	sprintf(m_szBuff,"应收:%.02f",ShiftTotal);
// 			printer->nPrintLine( m_szBuff );
//    	sprintf(m_szBuff,"实收:%.02f",ShiftTotal-ShiftrealTotal);
// 			printer->nPrintLine( m_szBuff );
// 
// 
//  	//取得收费换班金额
// 	double poschargeamt = 0;
// 	StyleID[0] = 0;
// 	for (i=1; i<10; i++)
// 	{
// 		StyleID[i] = i + 1;
// 	}
// 	getcadamt(shift_no,StyleID);
// 	printer->nPrintLine( "  " );
// 	sprintf(m_szBuff, "其它业务收入：   %11.2f",cadamt[0]+cadamt[2]);
// 	printer->nPrintLine(m_szBuff);
// 	sprintf(m_szBuff, "其它业务收入冲销:%11.2f",cadamt[1]);
// 	printer->nPrintLine(m_szBuff);
// 	poschargeamt = poschargeamt+ cadamt[0]+cadamt[2] - cadamt[1];
// 
// 
// 	CFinDataManage finDataManage;
// 	vector<POSChargeItemStyle> vecPOSChargeItemStyle;
// 	finDataManage.GetPOSChargeStyleID(vecPOSChargeItemStyle);
// 	bool bSKSell = false;
// 	bool bCZKSell = false;
// 	for ( vector<POSChargeItemStyle>::const_iterator sid = vecPOSChargeItemStyle.begin(); sid != vecPOSChargeItemStyle.end(); ++sid)
// 	{
// 		if (SID_MTKCHARGE == sid->StyleID)
// 		{
// 			bSKSell = true;
// 		}
// 		else if (SID_CZKCHARGE == sid->StyleID)
// 		{
// 			bCZKSell = true;
// 		}
// 	}
// 	//换班散卡销售统计
// 	if (bSKSell)
// 	{
// 		StyleID[0] = 1;
// 		for (i=1; i<10; i++)
// 		{
// 			StyleID[i] = -1;
// 		}
// 		getcadamt(shift_no,StyleID);
// 		printer->nPrintLine( "  " );
// 		sprintf(m_szBuff, "散卡销售金额：   %11.2f",(cadamt[0]+cadamt[2]));
// 		printer->nPrintLine(m_szBuff);
// 		sprintf(m_szBuff, "散卡退货金额:    %11.2f",cadamt[1]);
// 		printer->nPrintLine(m_szBuff);
// 		poschargeamt = poschargeamt+ cadamt[0]+cadamt[2] - cadamt[1];
// 	}
// 	//换班银川储值卡销售统计
// 	if (bCZKSell)
// 	{
// 		for (i=0; i<10; i++)
// 		{
// 			StyleID[i] = 11;
// 		}
// 		getcadamt(shift_no, StyleID);
// 		printer->nPrintLine( "  " );
// 		sprintf(m_szBuff, "银川预售销售金额：   %2.2f",(cadamt[0]+cadamt[2]));
// 		printer->nPrintLine(m_szBuff);
// 		sprintf(m_szBuff, "银川预售退货金额:    %5.2f",cadamt[1]);
// 		printer->nPrintLine(m_szBuff);
// 		poschargeamt = poschargeamt+ cadamt[0]+cadamt[2] - cadamt[1];
// 	}
   
/*	getcadamt(shift_no,2);
	printer->nPrintLine( "  " );
	sprintf(m_szBuff, "手机充值收费收入：   %11.2f",cadamt[0]+cadamt[2]);
	printer->nPrintLine(m_szBuff);
	sprintf(m_szBuff, "手机充值收费收入冲销:%11.2f",cadamt[1]);
	printer->nPrintLine(m_szBuff);

	poschargeamt = poschargeamt+ cadamt[0]+cadamt[2];*/

// 	printer->nPrintLine( "  " );
// 	sprintf(m_szBuff,"合计应收:%.02f",ShiftTotal+poschargeamt);
// 			printer->nPrintLine( m_szBuff );
//    	sprintf(m_szBuff,"合计实收:%.02f",ShiftTotal-ShiftrealTotal+poschargeamt);

	/*printer->nPrintLine( "  " );
	sprintf(m_szBuff, "其它业务收入：   %11.2f",cadamt[0]+cadamt[2]);
	printer->nPrintLine(m_szBuff);
	sprintf(m_szBuff, "其它业务收入冲销:%11.2f",cadamt[1]);
	printer->nPrintLine(m_szBuff);

	printer->nPrintLine( "  " );
	sprintf(m_szBuff,"合计应收:%.02f",ShiftTotal+cadamt[0]);
			printer->nPrintLine( m_szBuff );
   	sprintf(m_szBuff,"合计实收:%.02f",ShiftTotal-ShiftrealTotal+cadamt[0]);
	printer->nPrintLine( m_szBuff );*/
	//CString hh;hh.Format("印花总数:%d",GetShiftTotal_STAMPCOUNT(m_uShiftNo));//“刀具换购活动” 营销部对信息中心的需求
	//printer->nPrintLine(hh );


//	for (ti=0;ti<20;ti++) free(title[ti]);
	//delete []title;
	
	//打印横线
	printer->nPrintHorLine();

	//for ( int j=0;j < 4;j++) {	printer->nPrintLine(" "); }  //mabh
	//for (int j=0;j<GetSalePos()->GetParams()->m_nPrintFeedNum;j++)
	//{
	//	printer->nPrintLine("   ");
	//}
	printer->FeedPaper_End(PBID_SHIFT_BILL);

	//切纸 [dandan.wu 2016-3-7]
	printer->nEndPrint_m();
	GetSalePos()->GetWriteLog()->WriteLog("CSaleImpl::PrintShiftBill(),切纸",POSLOG_LEVEL1);

	vectcadshift.clear();
	vectpaycadshift.clear();
	//printer->nSetPrintBusiness(PBID_TRANS_RECEIPT);

	return true;

}

//thinpon. 更新班次开始时间为当前时间
BOOL CSaleImpl::UpdateShiftBeginTime()
{
	CString strSQL;
//	unsigned int ShiftNO;
	CString var;
		
	CPosClientDB *pDB = CDataBaseManager::getConnection(true);
	if ( !pDB )
		return false;
	
	try {
		strSQL.Format("UPDATE SHIFT SET Shift_Date = getdate()");
		pDB->Execute2( strSQL);//aori change 2011-3-22 16:51:36 优化clientdb ：删除execute2
		
	} catch( CException * e ) {
		CUniTrace::Trace(*e);
		e->Delete( );
		CDataBaseManager::closeConnection(pDB);
		return false;
	}

	CDataBaseManager::closeConnection(pDB);
	return true;		
}
//thinpon 获取开始班次时间
BOOL CSaleImpl::GetShiftBeginTime(SYSTEMTIME &_tm)
{
	CString strSQL;
//	unsigned int ShiftNO;
	//SYSTEMTIME m_stSaleDT;
	CRecordset* pRecord = NULL;
	CDBVariant var;
	
	CPosClientDB *pDB = CDataBaseManager::getConnection(true);
	if ( !pDB )
		return false;
	
	try {
		strSQL.Format("SELECT Shift_Date FROM Shift ");
		/*WHERE Shift_No = %d",GetSalePos()->GetCasher()->GetUserID()*/
		if ( pDB->Execute(strSQL, &pRecord) > 0 ) {
			pRecord->GetFieldValue((short)0, var);
			TStoST(*var.m_pdate, /*m_stSaleDT*/_tm);
		}
	} catch( CException * e ) {
		CUniTrace::Trace(*e);
		e->Delete( );
		CDataBaseManager::closeConnection(pDB);
		return false;
	}

	CDataBaseManager::closeConnection(pDB);
	return true;
}
/* 获取班次的首笔销售和最后一笔销售 */
int CSaleImpl::GetFirstSaleID(ULONG shift_no)
{
	CString strSQL;
//	unsigned int ShiftNO;
	//SYSTEMTIME m_stSaleDT;
	CRecordset* pRecord = NULL;
	CDBVariant var;
	
	CPosClientDB *pDB = CDataBaseManager::getConnection(true);
	if ( !pDB )
		return 0;
	
	try {
		strSQL.Format("SELECT isnull(min(saleid),0) FROM Sale%s where shift_no=%d",GetSalePos()->GetTableDTExt(&m_stSaleDT),m_uShiftNo);
		if ( pDB->Execute(strSQL, &pRecord) > 0 ) {
			pRecord->GetFieldValue((short)0, var);
						
		}else{ return 0; }
	} catch( CException * e ) {
		CUniTrace::Trace(*e);
		e->Delete( );
		CDataBaseManager::closeConnection(pDB);
		return 0;
	}

	CDataBaseManager::closeConnection(pDB);
	return var.m_lVal;
}

int CSaleImpl::GetLastSaleID(ULONG shift_no)
{
	CString strSQL;
//	unsigned int ShiftNO;
	//SYSTEMTIME m_stSaleDT;
	CRecordset* pRecord = NULL;
	CDBVariant var;
	
	CPosClientDB *pDB = CDataBaseManager::getConnection(true);
	if ( !pDB )
		return 0;
	
	try {
		strSQL.Format("SELECT ISNULL(max(saleid),0) FROM Sale%s where shift_no=%d",GetSalePos()->GetTableDTExt(&m_stSaleDT),m_uShiftNo);
		if ( pDB->Execute(strSQL, &pRecord) > 0 ) {
			pRecord->GetFieldValue((short)0, var);
						
		}else{ return 0;}
		
	} catch( CException * e ) {
		CUniTrace::Trace(*e);
		e->Delete( );
		CDataBaseManager::closeConnection(pDB);
		return 0;
	}

	CDataBaseManager::closeConnection(pDB);
	return var.m_lVal;
}

//获取班次销售单数
ULONG CSaleImpl::GetShiftTotalSale(ULONG shift_no)
{
	CString strSQL;
//	unsigned int ShiftNO;
	//SYSTEMTIME m_stSaleDT;
	CRecordset* pRecord = NULL;
	CDBVariant var;
	
	CPosClientDB *pDB = CDataBaseManager::getConnection(true);
	if ( !pDB )
		return 0;
	
	try {
		strSQL.Format("SELECT ISNULL(Count('X'),0) FROM Sale%s where shift_no=%d",GetSalePos()->GetTableDTExt(&m_stSaleDT),m_uShiftNo);
		if ( pDB->Execute(strSQL, &pRecord) > 0 ) {
			pRecord->GetFieldValue((short)0, var);
						
		}else{ return 0;}
		
	} catch( CException * e ) {
		CUniTrace::Trace(*e);
		e->Delete( );
		CDataBaseManager::closeConnection(pDB);
		return 0;
	}

	CDataBaseManager::closeConnection(pDB);
	return var.m_lVal;
}//获取班次印花数“刀具换购活动” 营销部对信息中心的需求
ULONG CSaleImpl::GetShiftTotal_STAMPCOUNT()
{
	CString strSQL;
//	unsigned int ShiftNO;
	//SYSTEMTIME m_stSaleDT;
	CRecordset* pRecord = NULL;
	CDBVariant var;
	
	CPosClientDB *pDB = CDataBaseManager::getConnection(true);
	if ( !pDB )
		return 0;
	
	try {
		CString strSpanBegin = GetDaySpanString(NULL,true);
		CString	strSpanEnd = GetDaySpanString(NULL, FALSE);
			strSQL.Format("SELECT ISNULL(sum(stampcount),0) FROM Sale%s"
			" where Status = 1 AND SaleDT >= '%s' AND SaleDT < '%s' "
			,GetSalePos()->GetTableDTExt(&m_stSaleDT),strSpanBegin,strSpanEnd);

		//	CUniTrace::Trace(strSQL);
		if ( pDB->Execute(strSQL, &pRecord) > 0 ) {
			pRecord->GetFieldValue((short)0, var);
						
		}else{ return 0;}
		
	} catch( CException * e ) {
		CUniTrace::Trace(*e);
		e->Delete( );
		CDataBaseManager::closeConnection(pDB);
		return 0;
	}

	CDataBaseManager::closeConnection(pDB);
	return var.m_lVal;
}


//======================================================================
// 函 数 名：GetShiftTotalSaleItem
//获得班次销售商品数
// 输入参数：ULONG shift_no
// 输出参数：bool
// 创建日期：00-2-21
// 修改日期：2009-4-1 曾海林
// 作  者：***
// 附加说明：数据库变更修改 SaleItem
//======================================================================
ULONG CSaleImpl::GetShiftTotalSaleItem(ULONG shift_no)
{
	CString strSQL;
//	unsigned int ShiftNO;
	//SYSTEMTIME m_stSaleDT;
	CRecordset* pRecord = NULL;
	CDBVariant var;
	
	CPosClientDB *pDB = CDataBaseManager::getConnection(true);
	if ( !pDB )
		return 0;
	
	try {
		strSQL.Format("SELECT ISNULL(Sum(b.SaleCount),0) FROM Sale%s a,SaleItem%s b where a.shift_no=%d and a.SaleID=b.SaleID",GetSalePos()->GetTableDTExt(&m_stSaleDT),GetSalePos()->GetTableDTExt(&m_stSaleDT),m_uShiftNo);
		if ( pDB->Execute(strSQL, &pRecord) > 0 ) {
			pRecord->GetFieldValue((short)0, var);
						
		}else{ return 0;}
		
	} catch( CException * e ) {
		CUniTrace::Trace(*e);
		e->Delete( );
		CDataBaseManager::closeConnection(pDB);
		return 0;
	}

	CDataBaseManager::closeConnection(pDB);
	return var.m_lVal;
}

//thinpon. 获取现金、支票等支付金额合计
ULONG CSaleImpl::GetShiftPaymentAmt(ULONG shift_no,char **title,float *amt,int col,int row)
{
	CString strSQL;
//	unsigned int ShiftNO;
	//SYSTEMTIME m_stSaleDT;
	CRecordset* pRecord = NULL;
	CDBVariant var;
	ULONG numRec=0;
	
	CPosClientDB *pDB = CDataBaseManager::getConnection(true);
	if ( !pDB )
		return 0;
	
	try {
		strSQL.Format("SELECT T2.Description,ISNULL(sum(T1.PaymentAmt),0)  FROM Payment%s T1,PayMentStyle T2 where T1.PSID=T2.PSID and T1.shift_no=%d  and T1.saledt > getdate()-3 group by T2.Description",GetSalePos()->GetTableDTExt(&m_stSaleDT),m_uShiftNo);
		numRec = pDB->Execute(strSQL, &pRecord);
		if (  numRec > 0 ) {
			int j = 0;
			for ( ; !pRecord->IsEOF() ; pRecord->MoveNext() ) {
				if (j >= numRec) break;
				pRecord->GetFieldValue((short)0, var);
				strcpy(title[j],*var.m_pstring);

				pRecord->GetFieldValue(1, var);
				amt[j] = atof(*var.m_pstring);
				j++;
			}
						
		}else{ CDataBaseManager::closeConnection(pDB);return 0;}
		
	} catch( CException * e ) {
		CUniTrace::Trace(*e);
		e->Delete( );
		CDataBaseManager::closeConnection(pDB);
		return 0;
	}

	CDataBaseManager::closeConnection(pDB);
	return numRec;
	
	return 0;
}
/***********************************************************************/
//thinpon. 新班次是否有销售(是否需要换班)
BOOL CSaleImpl::IsNeedChangeUser()
{
	CString strSQL;
	CRecordset* pRecord = NULL;
	CDBVariant var;
	BOOL flg;
	
	CPosClientDB *pDB = CDataBaseManager::getConnection(true);
	if ( !pDB )
		return false;
	
	try {
		strSQL.Format("SELECT COUNT('X')  FROM sale%s  where shift_no=%d ",GetSalePos()->GetTableDTExt(&m_stSaleDT),m_uShiftNo);
		if ( pDB->Execute(strSQL, &pRecord) > 0 ){
			pRecord->GetFieldValue((short)0, var);
			var.m_iVal > 0? flg=true:flg=false;
		}		
	} catch( CException * e ) {
		CUniTrace::Trace(*e);
		e->Delete( );
		CDataBaseManager::closeConnection(pDB);
		return false;
	}

	CDataBaseManager::closeConnection(pDB);
	return flg;

}
/*
//获取会员卡积分(远程)
long CSaleImpl::GetMemberIntergral()
{
	long ret=-1;

	//断网返回-1
	if (!GetSalePos()->GetParams()->m_bUseNet) return -1;
    //系统会员卡号返回-9
	if (!strcmp(GetSalePos()->GetParams()->m_sSystemVIP,GetMemberCode())) return -9;

	CPosClientDB* pDB = CDataBaseManager::getConnection(true);
	if ( !pDB ) {
		CUniTrace::Trace("CSaleImpl::GetMemberIntergral");
		CUniTrace::Trace(MSG_CONNECTION_NULL, IT_ERROR);
		//出错
		return -99;
	}

	CString strSQL;
	
	strSQL.Format("SELECT TotIntegral FROM PosServer..dbo.memeber WHERE MemberCode = %s", 
		GetMemberCode());
	CRecordset* pRecord = NULL;
	CDBVariant var;
	try {
		if ( pDB->Execute(strSQL, &pRecord) > 0 ) {
			pRecord->GetFieldValue((short)0, var);
			//if ( var.m_lVal > 0 )
			if (pRecord->GetRecordCount() > 0){
				ret=atol(*var.m_pstring);
			}
			else 
				//无此卡
				ret=-10;
			pDB->CloseRecordset(pRecord);
		}
	} catch (CException* e) {
		CUniTrace::Trace("CSaleImpl::GetMemberIntergral");
		CUniTrace::Trace(*e);
		e->Delete();}
	CDataBaseManager::closeConnection(pDB);
	
	return ret;
}*/
//======================================================================
// 函 数 名：PrintBillBootomSmall
// 功能描述：打印
// 输入参数：
// 输出参数：void
// 创建日期：2009-4-5 
// 修改日期：
// 作  者：曾海林
// 附加说明：
//======================================================================
void CSaleImpl::PrintBillBootomSmall()
{
	//打印机不可用则退出
	int printBusinessID=PBID_TRANS_RECEIPT;
	if(m_ServiceImpl.HasSVMerch())
		printBusinessID=PBID_SERVICE_RECEIPT;

	if (!GetSalePos()->GetPrintInterface()->m_bValid) 
		return;
	//记录日志
	//GetSalePos()->GetWriteLog()->WriteLog("开始调用打印小票尾",4);//[TIME OUT(Second) 0.000065]
	TPrintBottom pTPrintBottom;
	TPrintBmp pTPrintBmp;

	//打印销售小票条码bmp
	if (GetSalePos()->GetParams()->m_nPrintSaleBarcode == 1) {
		//打印bmp
		CString tmp,tmp_sale;
		strcpy(pTPrintBmp.bmp_name,"sale_barcode.bmp");
		GetSalePos()->GetWriteLog()->WriteLog(pTPrintBmp.bmp_name,4);
		tmp_sale.Format("%d",m_stSaleDT.wYear);
		tmp_sale=tmp_sale.Right(2);
		tmp.Format("%2s%02d%02d%04d%03d%06d",	tmp_sale, m_stSaleDT.wMonth, m_stSaleDT.wDay,
					m_pParam->m_nOrgNO , (int)m_pParam->m_nPOSID, m_nSaleID);
		strcpy(pTPrintBmp.barcodenum,tmp);
		GetSalePos()->GetWriteLog()->WriteLog(pTPrintBmp.barcodenum,4);

		GetSalePos()->GetPrintInterface()->nPrintBmp_m(&pTPrintBmp,printBusinessID);

	}

	//char *pt = m_szBuff;//aori del
	GetPrintFormatAlignString(GetSalePos()->GetPrintInterface()->strBOTTOM,m_szBuff,DT_CENTER);
	strcpy(pTPrintBottom.szMessage,m_szBuff);
	GetSalePos()->GetPrintInterface()->nPrintBottom_m(&pTPrintBottom,printBusinessID);

	//打印电商小票
	if(IsEMSale() || IsEOrder())
	{
		GetSalePos()->GetPrintInterface()->nPrintHorLine(); 
		CEorderinterface* m_eorderinterface = GetSalePos()->GetEorderinterface();
		CStringArray printArray;
		m_eorderinterface->GetPrintArray(&printArray);
		GetSalePos()->GetPrintInterface()->nPrintBill(&printArray);
	}

	//百安居没有此需求[dandan.wu 2016-3-15]
	//打印支票存根
	//PrintBillTotalCG();

	//打印储值卡第一联
	PrintBillTotalPrepaid();
	//打印米雅第一联
	PrintBillTotalMiya();

	//记录日志
	GetSalePos()->GetWriteLog()->WriteLog("开始调用打印优惠券",4);//[TIME OUT(Second) 0.000065]
	//广众通达优惠接口付款方式 zenghl@nes.com.cn on 20090411
	CCouponImpl *CouponImpl=GetSalePos()->GetCouponImpl();
	if (CouponImpl!=NULL)
	{
			if (CouponImpl->m_bValid)
			{
				CouponImpl->SetSaleImpl(this);
				HANDLE hPrint = NULL;
				CString strTmp=m_PayImpl.GetAllPayment();//支付方式
				try 
				{	
						CouponImpl->PrintBill(hPrint,strTmp);
				}
				catch (CException* e)
				{
					CUniTrace::Trace(*e);
					e->Delete( );	
				}
				
			}
	}
	//************************************************************
	/*for ( vector<PayInfo>::const_iterator pay = m_PayImpl.m_vecPay.begin(); pay != m_PayImpl.m_vecPay.end(); ++pay) {
	
		if ((stricmp (m_PayImpl.GetPaymentStyle(pay->nPSID)->szInterfaceName,"ICBC.dll")) == 0 )//能在小票上打印
		{	
			//记录日志
			GetSalePos()->GetWriteLog()->WriteLog("开始调用打印工行支付联",4);
			//工行打印
			GetSalePos()->GetPrintInterface()->nPrintBill(&GetSalePos()->GetPaymentInterface()->ArrayBill);

		}
	
				
    }//end if for ( vector<PayInfo>  mabh*/
	/*if (stricmp (ps->szInterfaceName,"ICBC.dll") == 0) {
		
		//记录日志
		GetSalePos()->GetWriteLog()->WriteLog("开始调用打印工行支付联",4);//[TIME OUT(Second) 0.000065]
		//工行打印
		GetSalePos()->GetPrintInterface()->nPrintBill(&GetSalePos()->GetPaymentInterface()->ArrayBill);
	}*/
		//记录日志
	
	//GetSalePos()->GetWriteLog()->WriteLog("开始调用打印工行支付联",4);//[TIME OUT(Second) 0.000065]
	//工行打印
	//GetSalePos()->GetPrintInterface()->nPrintBill(&GetSalePos()->GetPaymentInterface()->ArrayBill);
	if(m_ServiceImpl.HasSVMerch())
		GetSalePos()->GetPrintInterface()->nSetPrintBusiness(PBID_SERVICE_RECEIPT);
	else
		GetSalePos()->GetPrintInterface()->nSetPrintBusiness(PBID_TRANS_RECEIPT);
	GetSalePos()->GetPrintInterface()->FeedPaper_End(printBusinessID);

	PrintBillTotalPrepaid(true);
	
	PrintBillTotalMiya(true);

	GetSalePos()->GetPrintInterface()->nEndPrint_m();
	GetSalePos()->GetWriteLog()->WriteLog("CSaleImpl::PrintBillBootomSmall(),切纸",POSLOG_LEVEL1);

	return;

}
//======================================================================
// 函 数 名：PrintBillHeadSmall
// 功能描述：打印
// 输入参数：
// 输出参数：void
// 创建日期：2009-4-5 
// 修改日期：
// 作  者：曾海林
// 附加说明：
//======================================================================
void CSaleImpl::PrintBillHeadSmall()
{
	if (!GetSalePos()->GetPrintInterface()->m_bValid) 
		return;//打印机不可用则退出
	//GetSalePos()->GetWriteLog()->WriteLog("开始打印表头",4);////记录日志[TIME OUT(Second) 0.000065]
	TPrintHead	pPrintHead;//char *pt = m_szBuff;//aori del
	GetPrintFormatAlignString(GetSalePos()->GetPrintInterface()->strTOP,m_szBuff,DT_CENTER);
	strcpy(pPrintHead.szMessage,m_szBuff);  //抬头信息(商店名称)

	pPrintHead.nSaleID=m_nSaleID;			//最后一笔保存的销售流水号

	/************************************************************************/
	/*Modified by dandan.wu 2017-08-16
	*m_stSaleDT:插入临时表的SaleItem_Temp的时间
	*m_stEndSaleDT:插入月表SaleItem%s的时间，应该也是打印小票的时间
	*/
	/************************************************************************/
// 	sprintf(m_szBuff, "日期:%04d%02d%02d %02d	:%02d", 
// 	m_stSaleDT.wYear, m_stSaleDT.wMonth, m_stSaleDT.wDay, m_stSaleDT.wHour, m_stSaleDT.wMinute);
// 	pPrintHead.stSaleDT=m_stSaleDT;			//日期/时间
	sprintf(m_szBuff, "日期:%04d%02d%02d %02d	:%02d", 
		m_stEndSaleDT.wYear, m_stEndSaleDT.wMonth, m_stEndSaleDT.wDay, m_stEndSaleDT.wHour, m_stEndSaleDT.wMinute);
	pPrintHead.stSaleDT=m_stEndSaleDT;			
	
	pPrintHead.nUserID=atoi(GetSalePos()->GetCasher()->GetUserCode());

	//会员号
	memcpy(pPrintHead.szMemberCode,GetMember().szMemberCode,strlen(GetMember().szMemberCode));

	//会员姓名
	memcpy(pPrintHead.szMemberName,GetMember().szMemberName,strlen(GetMember().szMemberName));

	if(m_ServiceImpl.HasSVMerch())
	{
		GetSalePos()->GetPrintInterface()->FeedPaper_Head(PBID_SERVICE_RECEIPT);
		GetSalePos()->GetPrintInterface()->nPrintHead_m(&pPrintHead,PBID_SERVICE_RECEIPT);
	}
	else
	{
		GetSalePos()->GetPrintInterface()->FeedPaper_Head(PBID_TRANS_RECEIPT);
		GetSalePos()->GetPrintInterface()->nPrintHead_m(&pPrintHead,PBID_TRANS_RECEIPT);
	}
    return;
}
//======================================================================
// 函 数 名：PrintBillItemSmall
// 功能描述：打印相关设备
// 输入参数：
// 输出参数：void
// 创建日期：2009-4-5 
// 修改日期：
// 作  者：曾海林
// 附加说明：
//======================================================================
// void CSaleImpl::PrintBillItemSmall(CString szPLU, CString szSimpleName, double qty, double price, double amnt)
// {
// 	if (!GetSalePos()->GetPrintInterface()->m_bValid) return;//打印机不可用则退出
// 	SaleMerchInfo pSaleItem;strcpy(pSaleItem.szSimpleName,(LPTSTR)(LPCTSTR)szSimpleName);strcpy(pSaleItem.szPLU,(LPTSTR)(LPCTSTR)szPLU);
// 
// 	pSaleItem.fSaleAmt=amnt;pSaleItem.fActualPrice=price;
// 	pSaleItem.fSaleQuantity=qty;
// 	GetSalePos()->GetPrintInterface()->nPrintSalesItem(&pSaleItem);//if (pSaleItem.nMerchState != RS_CANCEL)
// 	return;
// }
void CSaleImpl::PrintBillItemSmall(const SaleMerchInfo *pmerchinfo, double qty, double price)
{
	if (!GetSalePos()->GetPrintInterface()->m_bValid)
		return; //打印机不可用则退出
	SaleMerchInfo pSaleItem=*pmerchinfo; 
	//mabh 判断13,18码 不打印数量
	if (pSaleItem.nManagementStyle == 2 || pSaleItem.bFixedPriceAndQuantity) 
		pSaleItem.bPriceInBarcode = true;
 
	pSaleItem.fActualPrice  = price;
	pSaleItem.fSaleQuantity = qty;

	//折扣后单价 [dandan.wu 2017-4-12]
	if ( qty != 0 )
	{
		pSaleItem.dbAfterDisPrice = pSaleItem.fSaleAmt/qty;
	}

	if ( pSaleItem.dCondPromDiscount != 0 || pSaleItem.dMemDiscAfterProm != 0 || pSaleItem.fManualDiscount != 0 )
	{
		pSaleItem.bDiscount = true;
	}
 
	if(m_ServiceImpl.HasSVMerch())
		GetSalePos()->GetPrintInterface()->nPrintSalesItem_m(&pSaleItem,PBID_SERVICE_RECEIPT); 
	else

		GetSalePos()->GetPrintInterface()->nPrintSalesItem_m(&pSaleItem,PBID_TRANS_RECEIPT); 
	return;
}
///aori begin 2011-12-30 14:42:38 proj 2011-12-22-1 combine 2.75 with 2.71 saleimpl
/*
void CSaleImpl::PrintBillItemSmall_m(const SaleMerchInfo *pmerchinfo, double qty, double price,int PrintBusinessID){
	if (!GetSalePos()->GetPrintInterface()->m_bValid) return;
	SaleMerchInfo pSaleItem=*pmerchinfo;//打印机不可用则退出
	pSaleItem.fActualPrice  = price;
	pSaleItem.fSaleQuantity = qty;
	GetSalePos()->GetPrintInterface()->nPrintSalesItem_m(&pSaleItem,PrintBusinessID);//mabh
	return;
}//aori end 2011-12-30 14:42:38 proj 2011-12-22-1 combine 2.75 with 2.71 saleimpl
*/
//======================================================================
// 函 数 名：PrintBillTotalSmall
// 功能描述：打印
// 输入参数：
// 输出参数：void
// 创建日期：2009-4-5 
// 修改日期：//zenghl 20090520 增加远程积分打印
// 作  者：曾海林
// 附加说明：
//======================================================================
void CSaleImpl::PrintBillTotalSmall()
{
	PrintBillTotalSmall(&m_vecSaleMerch,false);
}
void CSaleImpl::PrintBillTotalSmall(const vector<SaleMerchInfo> *pVecSaleMerch,bool pPoolClothes)
{
	if (!GetSalePos()->GetPrintInterface()->m_bValid) 
		return; 

	int printBusinessID = PBID_TRANS_RECEIPT;
	if(m_ServiceImpl.HasSVMerch())
		printBusinessID = PBID_SERVICE_RECEIPT;

	GetSalePos()->GetPrintInterface()->nSetPrintBusiness(printBusinessID);
	bool printLine=false; 
	double  forchange_amt =0.0;
	double fTotal=0;
	if (!pPoolClothes)
	{
		GetSalePos()->GetPromotionInterface()->FormatPromoInfo_FromPromoTempTable(m_nSaleID);
		vector<TPromotion> vPromotion=GetSalePos()->GetPromotionInterface()->m_vPromotion;
		//促销 打印
		for ( vector<TPromotion>::iterator pPromotion = vPromotion.begin(); pPromotion != vPromotion.end(); ++pPromotion)
		{
			GetSalePos()->GetPrintInterface()->nPrintPromotion(pPromotion);
			printLine=true;
		}

		//促销 打印(重开小票时的)
		if ( vPromotion.size() == 0 ) 
		{ 
			vector<TPromotion> vreloadPromotion=GetSalePos()->GetPromotionInterface()->m_vreloadPromotion;
			for ( vector<TPromotion>::iterator preloadPromotion = vreloadPromotion.begin(); preloadPromotion != vreloadPromotion.end(); ++preloadPromotion)
			{
				GetSalePos()->GetPrintInterface()->nPrintPromotion(preloadPromotion);//打印促销
				printLine=true;
			}
			GetSalePos()->GetPromotionInterface()->m_vreloadPromotion.clear();  
		}
			//横线
//		if(printLine)
//			GetSalePos()->GetPrintInterface()->nPrintHorLine(); 

		//打印会员折扣总金额和节省总金额
		printBillSumDiscount();

		//打印合计
		TPrintTotal_SUM	pPrintTotale_sum;
		fTotal=GetTotalAmt_beforePromotion();
		//ReducePrecision2(fTotal,GetSalePos()->GetParams()->m_nPayPrecision);//条件促销时 应付金额 多扣 调用 GetTotalAmt_beforePromotion
		if ( strlen(m_MemberImpl.m_Member.szMemberCode) > 1 )
		{
			if (GetSalePos()->GetParams()->m_bWallet_Wipe_Zero)   //使用零钱包是否抹零  0:不抹零， 1：抹零
				ReducePrecision2(fTotal,GetSalePos()->GetParams()->m_nPayPrecision);//2:舍厘 1://舍分
		}
		else
			ReducePrecision2(fTotal,GetSalePos()->GetParams()->m_nPayPrecision);
	
		strcpy(pPrintTotale_sum.szMemberCode,GetMember().szMemberCode);

		pPrintTotale_sum.nMemberScore=GetMember().dTotIntegral;
		if (strcmp(GetSalePos()->GetParams()->m_sSystemVIP,GetMember().szMemberCode)==0)
			pPrintTotale_sum.isSystem=1;
		else
			pPrintTotale_sum.isSystem=0;
		pPrintTotale_sum.nHotScore=GetSalePos()->GetPromotionInterface()->m_nHotReducedScore;//减少积分
		pPrintTotale_sum.nTotalQty=GetTotalCount();
		pPrintTotale_sum.nTotalAmt = GetBNQTotalAmt();//fTotal;
		pPrintTotale_sum.fRebateAccAmt=GetMember().fRebateAccAmt;
		pPrintTotale_sum.fWalletAccAmt=0;
		strcpy(pPrintTotale_sum.szRebateAccBeginDate,GetMember().szRebateAccBeginDate);
		strcpy(pPrintTotale_sum.szRebateAccEndDate,GetMember().szRebateAccEndDate);
		GetSalePos()->GetPrintInterface()->nPrintTotal_Sum(&pPrintTotale_sum); 
	}
	else
	{
		double amt_beforeProm=GetTotalAmt_beforePromotion(pVecSaleMerch);
		double amt_afterProm=GetTotalAmt(pVecSaleMerch);
		double amt_discount = amt_beforeProm-amt_afterProm;
		ReducePrecision2(amt_discount,GetSalePos()->GetParams()->m_nPayPrecision);
		//打印条件促销优惠金额
		if(amt_discount>0)
		{
			//GetSalePos()->GetPrintInterface()->nPrintHorLine(); 
			sprintf(m_szBuff,"条件促销优惠金额: -%.2f", amt_discount);
			GetSalePos()->GetPrintInterface()->nPrintLine(m_szBuff);
		}
		//打印合计
		TPrintTotal_SUM	pPrintTotale_sum;
		double fTotal=amt_afterProm;
		//ReducePrecision2(fTotal,GetSalePos()->GetParams()->m_nPayPrecision);//条件促销时 应付金额 多扣 调用 GetTotalAmt_beforePromotion
		if ( strlen(m_MemberImpl.m_Member.szMemberCode) > 1 )
		{
			if (GetSalePos()->GetParams()->m_bWallet_Wipe_Zero)   //使用零钱包是否抹零  0:不抹零， 1：抹零
				ReducePrecision2(fTotal,GetSalePos()->GetParams()->m_nPayPrecision);//2:舍厘 1://舍分
		}
		else
			ReducePrecision2(fTotal,GetSalePos()->GetParams()->m_nPayPrecision);
		strcpy(pPrintTotale_sum.szMemberCode,GetMember().szMemberCode);
		pPrintTotale_sum.nMemberScore=GetMember().dTotIntegral;
		if (strcmp(GetSalePos()->GetParams()->m_sSystemVIP,GetMember().szMemberCode)==0)
			pPrintTotale_sum.isSystem=1;
		else
			pPrintTotale_sum.isSystem=0;
		pPrintTotale_sum.nHotScore=GetSalePos()->GetPromotionInterface()->m_nHotReducedScore;//减少积分
		pPrintTotale_sum.nTotalQty=GetTotalCount(pVecSaleMerch);
		pPrintTotale_sum.nTotalAmt=fTotal;
		pPrintTotale_sum.fRebateAccAmt=GetMember().fRebateAccAmt;
		pPrintTotale_sum.fWalletAccAmt=0;
		strcpy(pPrintTotale_sum.szRebateAccBeginDate,GetMember().szRebateAccBeginDate);
		strcpy(pPrintTotale_sum.szRebateAccEndDate,GetMember().szRebateAccEndDate);
		GetSalePos()->GetPrintInterface()->nPrintTotal_Sum(&pPrintTotale_sum); 
	}



//<------------------如果m_bPoolClothesMerch为true打印联营服饰商品的小票
	const TPaymentStyle *ps = NULL;
	if(!GetSalePos()->GetParams()->m_bPoolClothesMerch)  //服饰小票不打印以下内容
	{	
		double FaPiaoTotal=0.0;//aori add 2011-2-9 13:40:07 need 发票金额打印

		//printLine=false; 

		for ( vector<PayInfo>::const_iterator pay = m_PayImpl.m_vecPay.begin(); pay != m_PayImpl.m_vecPay.end(); ++pay) 
		{
 			TPaymentItem	pPaymentItem;
	
			ps = m_PayImpl.GetPaymentStyle(pay->nPSID);
			if ( NULL != ps && ps->bPrintOnVoucher)//能在小票上打印
			{	
				//从表中读取是否打印余额，比如储值卡需要打印余额 [dandan.wu 2016-3-17]
				pPaymentItem.bPrintRemainAmt = ps->bPrintRemainAmt; 

				pPaymentItem.bPrintOnVoucher = true;
				
				if ( ps->bNeedPayNO )
				{
					for ( vector<PayInfoItem>::iterator payInfoItem = m_PayImpl.m_vecPayItem.begin(); 
					payInfoItem != m_PayImpl.m_vecPayItem.end(); ++payInfoItem) 
					{
						if ( payInfoItem->nPSID == pay->nPSID ) 
						{
							pPaymentItem.nPSID = payInfoItem->nPSID;
							//支付类型名称
							strcpy(pPaymentItem.szDescription,ps->szDescription);
						
							//支付金额
							pPaymentItem.fPaymentAmt = payInfoItem->fPaymentAmt;
							
							//余额
							if (pPaymentItem.bPrintRemainAmt)
								pPaymentItem.fRemainAmt = payInfoItem->fRemainAmt;

							//号码
							strcpy(pPaymentItem.szPaymentNum,payInfoItem->szPaymentNum);

							//若为分期付款，需要打印贷款ID、贷款期数、类型 [dandan.wu 2016-3-28]
							if ( payInfoItem->nPSID == ::GetSalePos()->GetParams()->m_nInstallmentPSID )
							{
								pPaymentItem.nLoanID = payInfoItem->nLoanID;
								pPaymentItem.nLoanPeriod = payInfoItem->nLoanPeriod;
								if ( strlen(payInfoItem->szLoanType) == 0 )
								{
									CUniTrace::Trace("销售：贷款类型为空");
								}
								else
								{
									strcpy(pPaymentItem.szLoanType,payInfoItem->szLoanType);
								}

								if ( strlen(payInfoItem->szLoanID) == 0 )
								{
									CUniTrace::Trace("销售：贷款ID描述为空");
								}
								else
								{
									strcpy(pPaymentItem.szLoanID,payInfoItem->szLoanID);
								}
							}					
							else if (payInfoItem->nPSID == ::GetSalePos()->GetParams()->m_nPrivilegedDeductionPSID)
							{
								if ( strlen(payInfoItem->szExtraInfo) == 0 )
								{
									CUniTrace::Trace("销售：额外信息为空");
								}
								else
								{
									strcpy(pPaymentItem.szExtraInfo,payInfoItem->szExtraInfo);
								}
							}
							else if (payInfoItem->nPSID == ::GetSalePos()->GetParams()->m_nSavingCardPSID)
							{
								if ( strlen(payInfoItem->szExtraInfo) == 0 )
								{
									CUniTrace::Trace("销售：额外信息为空");
								}
								else
								{
									strcpy(pPaymentItem.szExtraInfo,payInfoItem->szExtraInfo);
								}
							}

							//aori add 银行卡打印隐蔽倒数第5至倒第12位
							HideBankCardNum(pay, pPaymentItem);
							
							if (pPaymentItem.fPaymentAmt > 0 )
		    						GetSalePos()->GetPrintInterface()->nPrintPayment(&pPaymentItem);//打印支付							
						}
					}
				}
				else
				{
					//金额
					strcpy(pPaymentItem.szDescription,ps->szDescription);
					pPaymentItem.fPaymentAmt = pay->fPayAmt;
					
					if (!ps->bNeedReadCard) //非IC卡打paynum
							strcpy(pPaymentItem.szPaymentNum,pay->szPayNum);

					if (pPaymentItem.fPaymentAmt > 0 )
		    				GetSalePos()->GetPrintInterface()->nPrintPayment(&pPaymentItem);//打印支付
				}

				if ( ps->bForChange )
					forchange_amt -= pay->fPayAmt;
				
				if ( ps->bForInvoiceAmt )//aori add 2011-2-9 13:40:07 need 发票金额打印
					FaPiaoTotal += pay->fPayAmt;
				 		
		    //printLine=true;
			}	
		}
		//横线
		//if(printLine)
		//	GetSalePos()->GetPrintInterface()->nPrintHorLine(); 

		TPrintTotal_change pPrintTotale_change;
		double dTemp = m_PayImpl.GetChange();
		pPrintTotale_change.nPayAmt=m_PayImpl.GetPayAmt()+forchange_amt;
		pPrintTotale_change.nChange=dTemp;
		GetSalePos()->GetPrintInterface()->nPrintTotal_Change(&pPrintTotale_change);//打印找零
		
		//支付宝折扣金额
		double alipay_Discount=0;
		CString m_szDescription;
		ps = NULL;
		for ( vector<PayInfo>::const_iterator alipay = m_PayImpl.m_vecPay.begin(); alipay != m_PayImpl.m_vecPay.end(); ++alipay) 
		{
 			TPaymentItem	pPaymentItem;
			vector<PayInfoItem> vecFindPayItem;
			ps = m_PayImpl.GetPaymentStyle(alipay->nPSID);
			if ( NULL != ps && strcmp(ps->szInterfaceName,"PlatformPay.dll") == 0 )
			{
				//根据支付号取明细
				m_PayImpl.GetPayInfoItemVect(alipay->nPSID, &vecFindPayItem);

				for ( vector<PayInfoItem>::iterator payInfoItem = vecFindPayItem.begin(); payInfoItem != vecFindPayItem.end(); ++payInfoItem)
				{				

					if (payInfoItem->fRemainAmt > 0 )
					{
						alipay_Discount=alipay_Discount+payInfoItem->fRemainAmt;
						m_szDescription.Format("%s",ps->szDescription);
					}
				}
				//合计余额值作为折扣金额
		    //printLine=true;
			}	
		}
		//alipay_Discount=3.0; //test
		if (alipay_Discount > 0)
		{
			sprintf(m_szBuff, "%s折扣金额:%13.2f ",m_szDescription,-alipay_Discount);
			GetSalePos()->GetPrintInterface()->nPrintLine(m_szBuff);
			//sprintf(m_szBuff, "实际支付金额:%15.2f ",m_PayImpl.GetPayAmt()+forchange_amt-dTemp-alipay_Discount);
			//GetSalePos()->GetPrintInterface()->nPrintLine(m_szBuff);
			//横线
			//GetSalePos()->GetPrintInterface()->nPrintHorLine(); 
		}

		//打印找零充值
		ps = NULL;
		for ( vector<PayInfo>::const_iterator pay2 = m_PayImpl.m_vecPay.begin(); pay2 != m_PayImpl.m_vecPay.end(); ++pay2) 
		{
			TPaymentItem		pPaymentItem;
			ps = m_PayImpl.GetPaymentStyle(pay2->nPSID);
			if ( NULL != ps && ps->bPrintOnVoucher && ps->bForChange)
			{
				//能在小票上打印
				//先打印payment
				strcpy(pPaymentItem.szDescription,ps->szDescription);
				pPaymentItem.fPaymentAmt=-pay2->fPayAmt;
				pPaymentItem.bPrintRemainAmt = ps->bPrintRemainAmt;
				if (!ps->bNeedReadCard) //非IC卡打paynum
					strcpy(pPaymentItem.szPaymentNum,pay2->szPayNum);
				HideBankCardNum( pay2,pPaymentItem);//aori add 银行卡打印隐蔽倒数第5至倒第8位
				GetSalePos()->GetPrintInterface()->nPrintPayment(&pPaymentItem);//打印支付
			}	
		}
		//GetPrintChars(m_szBuff, nMaxLen,nMaxLen, true);
		//GetSalePos()->GetPrintInterface()->nPrintLine_m( m_szBuff ,printBusinessID);//横线
		//GetPrintChars(m_szBuff, nMaxLen,nMaxLen, true);GetSalePos()->GetPrintInterface()->nPrintLine( m_szBuff );//横线//aori 2011-12-30 14:42:38 proj 2011-12-22-1 combine 2.75 with 2.71 saleimpl
		
		//打印明细paymentitem IC卡
		printLine=false;
		ps = NULL;
		const TPaymentStyle *psItem = NULL;
		for ( vector<PayInfo>::const_iterator pay1 = m_PayImpl.m_vecPay.begin(); pay1 != m_PayImpl.m_vecPay.end(); ++pay1) 
		{
			TPaymentItem		pPaymentItem;
			vector<PayInfoItem> vecFindPayItem;

			ps = m_PayImpl.GetPaymentStyle(pay1->nPSID);

			//根据支付号取明细
			m_PayImpl.GetPayInfoItemVect(pay1->nPSID, &vecFindPayItem);
			if ( NULL != ps && ps->bNeedReadCard )
			{	
				//bNeedReadCard
				CString extp,strBuf;
				int prtline = -1;
				//读取打印行数
				GetSalePos()->GetWriteLog()->WriteLog("读取打印行数",3,POSLOG_LEVEL3); 
				extp.Format("%s",ps->szExtParams);
				strBuf.Format("支付号码行数：%s",extp);
				GetSalePos()->GetWriteLog()->WriteLog(strBuf,3,POSLOG_LEVEL3); 
				if (extp.GetLength() > 10 ) 
				{
					extp = get_xml_param(extp,"PrtLine");
					if (extp.GetLength() > 0)
						prtline = atoi(extp); 
				}
					
				int i = 0;
				for ( vector<PayInfoItem>::iterator payInfoItem = vecFindPayItem.begin(); payInfoItem != vecFindPayItem.end(); ++payInfoItem)
				{				
					pPaymentItem.fPaymentAmt = 0;//pPaymentItem.fPaymentAmt=payInfoItem->fPaymentAmt;
					//支付号码调整
					CString m_strPayNum,m_strPiPayNo;
					m_strPayNum=payInfoItem->szPaymentNum;
					if (m_strPayNum.GetLength() > 62)
					{
						int m_ncount=m_strPayNum.ReverseFind('-');
						if (m_ncount > 0)
						{
							m_strPiPayNo = m_strPayNum.Right(m_strPayNum.GetLength()-m_ncount-1);
							m_strPayNum=m_strPayNum.Left(m_ncount);
						}
					}

					strcpy(pPaymentItem.szPaymentNum,m_strPayNum);

					psItem = m_PayImpl.GetPaymentStyle(payInfoItem->nPSID);
					if ( NULL != psItem )
					{
						strcpy(pPaymentItem.szDescription,psItem->szDescription);
						pPaymentItem.bPrintRemainAmt = psItem->bPrintRemainAmt;
					}
					
					if(pPaymentItem.bPrintRemainAmt )	
					{
						pPaymentItem.fRemainAmt=payInfoItem->fRemainAmt;
						pPaymentItem.stSaleDT=payInfoItem->stEffectDT;//有效期(ic卡使用)
					}
					if ( ps->bPrintOnVoucher )
					{
						//能在小票上打印
						HideBankCardNum( pay1,pPaymentItem);//aori add 银行卡打印隐蔽倒数第5至倒第8位
						i=i+1;
						if ( prtline < 0) 
						{
							strBuf.Format("不控制打印支付号码行数：%d",i);
							GetSalePos()->GetWriteLog()->WriteLog(strBuf,3,POSLOG_LEVEL3); 
							GetSalePos()->GetPrintInterface()->nPrintPayment(&pPaymentItem);//打印支付
						}
						else//有打印控制
						{
							if (  i < prtline ) 
							{
								strBuf.Format("打印支付号码行数：%d",i);
								GetSalePos()->GetWriteLog()->WriteLog(strBuf,3,POSLOG_LEVEL3); 
								GetSalePos()->GetPrintInterface()->nPrintPayment(&pPaymentItem);//打印支付
							}
							else 
							{ 
								if ( i == prtline ) 
								{
									strBuf.Format("打印支付号码行数：%d",i);
									GetSalePos()->GetWriteLog()->WriteLog(strBuf,3,POSLOG_LEVEL3); 
									GetSalePos()->GetPrintInterface()->nPrintPayment(&pPaymentItem);//打印支付
								}
								else
								{
									strBuf.Format("不需打印支付号码行数：%d",i);
									GetSalePos()->GetWriteLog()->WriteLog(strBuf,3,POSLOG_LEVEL3); 

								}
							}
						}//有打印控制
					}//能在小票上打印
					printLine=true;
				}//for
				if (prtline > 0 && i > prtline  )
				{
					strBuf.Format("有%d个支付号码省略打印!",(i-prtline));
					GetSalePos()->GetPrintInterface()->nPrintLine(strBuf);
				}

			}//bNeedReadCard				
		}
		//横线
		if(printLine)
			GetSalePos()->GetPrintInterface()->nPrintHorLine(); 
		//<--#服务调用接口#
		printLine=false;
		if(m_ServiceImpl.HasSVMerch() && m_ServiceImpl.PrintOnVoucher())
		{
			CStringArray msgArray;
			m_ServiceImpl.GetPrintMsg(&msgArray);
			for(int i=0;i<msgArray.GetSize();i++)
			{
				GetSalePos()->GetPrintInterface()->nPrintLine(msgArray.GetAt(i));
				printLine=true;
			}
		}
		//横线
		if(printLine)
			GetSalePos()->GetPrintInterface()->nPrintHorLine(); 
		//打印会员信息
		fTotal=GetTotalAmt();
		//ReducePrecision2(fTotal,GetSalePos()->GetParams()->m_nPayPrecision);
		if ( strlen(m_MemberImpl.m_Member.szMemberCode) > 1 )
		{
			if (GetSalePos()->GetParams()->m_bWallet_Wipe_Zero)   //使用零钱包是否抹零  0:不抹零， 1：抹零
				ReducePrecision2(fTotal,GetSalePos()->GetParams()->m_nPayPrecision);//2:舍厘 1://舍分
		}
		else
			ReducePrecision2(fTotal,GetSalePos()->GetParams()->m_nPayPrecision);

		TPrintTotal_SUM	pPrintTotale_sum;
		//pPrintTotale_sum.fMemXFSScore = m_MemXFSScore;
		strcpy(pPrintTotale_sum.szMemberCode,GetMember().szMemberCode);
		pPrintTotale_sum.nMemberScore = GetMember().dTotIntegral;
		if (strcmp(GetSalePos()->GetParams()->m_sSystemVIP,GetMember().szMemberCode)==0)
			pPrintTotale_sum.isSystem=1;
		else
			pPrintTotale_sum.isSystem=0;
		pPrintTotale_sum.nHotScore=GetSalePos()->GetPromotionInterface()->m_nHotReducedScore;//减少积分
		pPrintTotale_sum.nTotalQty=GetTotalCount(pVecSaleMerch);
		pPrintTotale_sum.fRebateAccAmt=GetMember().fRebateAccAmt;
		pPrintTotale_sum.fWalletAccAmt=0;
		strcpy(pPrintTotale_sum.szRebateAccBeginDate,GetMember().szRebateAccBeginDate);
		strcpy(pPrintTotale_sum.szRebateAccEndDate,GetMember().szRebateAccEndDate);
		//if (GetSalePos()->GetParams()->m_nUseRemoteMemScore==1||GetSalePos()->GetParams()->m_nUseRemoteMemScore==2
		//	||GetSalePos()->GetParams()->m_nUseRemoteMemScore==3 ||GetSalePos()->GetParams()->m_nUseRemoteMemScore==4){//zenghl 20090520
		//	pPrintTotale_sum.nMemberScore=GetSalePos()->GetPosMemberInterface()->m_fMemberRealtimeScore;//取远程积分
		//}
		//实时积分
		GetSalePos()->GetWriteLog()->WriteLog("打印积分",3);//日志
		if (GetSalePos()->GetParams()->m_bRealtimeJF == 1 && strlen(m_MemberImpl.m_Member.szMemberCode) > 1)
		{
			CString strbuf;

			//RealtimeJF  *m_RealtimeJF = GetSalePos()->GetRealtimeJF() ;

			pPrintTotale_sum.bScoreRealtime=true;
			pPrintTotale_sum.bScoreSuccess=m_MemberImpl.m_RealtimeScoreSuccess;
			//strbuf.Format("bScoreSuccess:%d",m_RealtimeJF->rtn_flag);
			//GetSalePos()->GetWriteLog()->WriteLog(strbuf,3);//日志
			pPrintTotale_sum.nAddScore=m_MemberImpl.m_dMemIntegral;
			//strbuf.Format("nAddScore:%d",m_RealtimeJF->rtn_jf);
			//GetSalePos()->GetWriteLog()->WriteLog(strbuf,3);//日志
			//GetSalePos()->GetRealtimeJF() ;
		}
		//pPrintTotale_sum.fMemXFSScore = m_MemXFSScore;//会员消费送积分
		//尚佳会员打印
		if(GetSalePos()->GetParams()->m_nUseRemoteMemScore == 5)
		{
			pPrintTotale_sum.isSystem = 2;
			GetSalePos()->GetPrintInterface()->nPrintMemberInfo(&pPrintTotale_sum);//打印会员信息
			//打印尚佳会员卡面号
			CString strBuf;
			if (strlen(m_MemberImpl.m_Member.szVIPCardNO) > 1)
			{
				strBuf.Format("%s:%s","会员卡面号", m_MemberImpl.m_Member.szVIPCardNO);
				GetSalePos()->GetPrintInterface()->nPrintLine(strBuf);
			}
			//打印尚佳会员姓名
			if (strlen(m_MemberImpl.m_Member.szMemberName) > 1)
			{
				strBuf.Format("%s", m_MemberImpl.m_Member.szMemberName);
				GetSalePos()->GetPrintInterface()->nPrintLine(strBuf);
			}
		}
		else
		{
			GetSalePos()->GetPrintInterface()->nPrintMemberInfo(&pPrintTotale_sum);//打印会员信息
		}
		//打印会员金银卡信息
		if (GetSalePos()->GetParams()->m_nGoldcardtype > 0 && strlen(m_MemberImpl.m_Member.szMemberCode) > 1)//可以打印
		{
			CString strBuf;
			if (GetSalePos()->GetParams()->m_nGoldcardtype == atoi(GetMember().szCardTypeCode))	 //金卡打印
			{
				strBuf.Format("%s", GetSalePos()->GetParams()->m_Goldcardprtmsg);
			}
			else	//银卡打印
			{
				strBuf.Format("%s", GetSalePos()->GetParams()->m_Silvercardprtmsg);
			}
			GetSalePos()->GetPrintInterface()->nPrintLine(strBuf);

		}
		
		//打印会员消费送积分
		if (GetSalePos()->GetParams()->m_bMemberXFSScore && strlen(m_MemberImpl.m_Member.szMemberCode) > 1)
		{
			CString strBuf;
			strBuf.Format("会员消费送赠送积分:%.2f", m_MemberImpl.m_dMemXFSScore);
			GetSalePos()->GetPrintInterface()->nPrintLine(strBuf);
			GetSalePos()->GetPrintInterface()->nPrintHorLine();
		}
		//打印会员中奖信息
		if (GetSalePos()->GetParams()->m_bMemberLottery && strlen(m_MemberImpl.m_Member.szMemberCode) > 1)
		{
			if (m_MemberImpl.m_LotteryGrade)
			{
				CString strMemberLottery;
				strMemberLottery.Format("中奖等级:%d",m_MemberImpl.m_LotteryGrade);
				GetSalePos()->GetPrintInterface()->nPrintLine(strMemberLottery);
				GetSalePos()->GetPrintInterface()->nPrintHorLine(); 
			}
			else
			{
				CString strMemberLottery;
				strMemberLottery.Format("中奖等级:");
				GetSalePos()->GetPrintInterface()->nPrintLine(strMemberLottery);
				GetSalePos()->GetPrintInterface()->nPrintHorLine(); 
			}
		}
		//打印电商支付码，和电商会员号
		if (IsEOrder())
		{
			if (m_EorderID)
			{
				CString strEorderID;
				strEorderID.Format("电商订单号:%s",m_EorderID);
				GetSalePos()->GetPrintInterface()->nPrintLine(strEorderID);
			}
			/*
			if (GetSalePos()->m_params.m_EOrderMemberCode)
			{
					CString strEOrderMemberCode;
					strEOrderMemberCode.Format("电商订单号:%s",GetSalePos()->m_params.m_EOrderMemberCode);
					GetSalePos()->GetPrintInterface()->nPrintLine(strEOrderMemberCode);
			}*/
		}
		
		if (IsEMSale())
		{
			/*
			if (m_EMemberID)
			{
				CString strEMemberID;
				strEMemberID.Format("电商会员号:%s",m_EMemberID);
				GetSalePos()->GetPrintInterface()->nPrintLine(strEMemberID);
			}*/
			if (m_EorderID)
			{
				CString strEorderID;
				strEorderID.Format("电商订单号:%s",m_EorderID);
				GetSalePos()->GetPrintInterface()->nPrintLine(strEorderID);
			}
		}
		//aori add 2011-2-9 13:40:07 need 发票金额打印 Begin
		//double dTemp = m_PayImpl.GetChange();
		FaPiaoTotal-=dTemp;//aori add 2011-2-9 13:40:07 bug:发票额为支付额  2011-2-18 11:27:57
		//FaPiaoTotal=ReducePrecision(FaPiaoTotal);
		//ReducePrecision2(FaPiaoTotal,GetSalePos()->GetParams()->m_nPayPrecision);
		if(GetSalePos()->GetParams()->m_bPrintInvoiceAmt)
		{
			//AfxMessageBox("打印发票金额");
			CString Num[]={"零","壹","贰","叁","肆","伍","陆","柒","捌","玖"};
			CString Wei[]={"整","分","角","元","拾","佰","仟","万","亿"};
			double tmptotal=(FaPiaoTotal-alipay_Discount)*100;
			CString DaXie,pre;
			pre.Format("整");
			DaXie.Format("整");
			bool bneedzero=false,bhasNUM=false;
			for(int j=0;tmptotal!=0;j++)
			{
				int i=((int)tmptotal)%10;
				tmptotal=tmptotal/10;
				if(i==0)
				{
					bhasNUM?bneedzero=true:1;
					if(j==2)
					{
						DaXie="元"+pre;
						bneedzero=FALSE;
					};
				}
				else
				{
					bhasNUM=true;
					bneedzero?(DaXie=Num[i]+Wei[1+j]+"零"+pre,bneedzero=false):
						  DaXie=Num[i]+Wei[1+j]+pre;			
				}
				pre=DaXie;
			}
			CString printfapiao;printfapiao.Format("发票金额小写:%.2f元 \n发票金额大写:%s",FaPiaoTotal-alipay_Discount,DaXie);
			//AfxMessageBox(printfapiao);
		//	GetSalePos()->GetPrintInterface()->nPrintLine( printfapiao );//aori del 2011-12-30 14:42:38 proj 2011-12-22-1 combine 2.75 with 2.71 saleimpl
			GetSalePos()->GetPrintInterface()->nPrintLine( printfapiao);//	2011-12-30 14:42:38 proj 2011-12-22-1 combine 2.75 with 2.71 saleimpl
			GetSalePos()->GetPrintInterface()->nPrintHorLine(); 
		}
	//aori add 2011-2-9 13:40:07 need 发票金额打印 end
	}
	return;
}


void CSaleImpl::PrintBillTotalCG()
{	//打印支票存根
	if(!GetSalePos()->GetPrintInterface()->m_bValid)
		return;
	if(!GetSalePos()->GetPrintInterface()->GetPOSPrintConfig(PBID_CHECK_STUB).Enable) 
		return;
	//GetSalePos()->GetWriteLog()->WriteLog("开始打印支票存根",4);//[TIME OUT(Second) 0.000065]
	bool bp=false;
	bool bNeedSign = false;
	for ( vector<PayInfo>::const_iterator pay_tmp = m_PayImpl.m_vecPay.begin(); pay_tmp != m_PayImpl.m_vecPay.end(); ++pay_tmp) 
	{
		vector<PayInfoItem> vecFindPayItem;
		m_PayImpl.GetPayInfoItemVect(pay_tmp->nPSID, &vecFindPayItem);	
/*		if (m_PayImpl.GetPaymentStyle(pay_tmp->nPSID)->bNeedPayNO)
		{
			bp=true;
			break;//如果无可打印的，返回
		}*/
		if (pay_tmp->nPSID == GetSalePos()->m_params.m_nCheckAppPSID)
		{
			bp=true;
			break;
		}
	}		
	if(!bp)
		return;//aori recover 
	CPrintInterface  *printer = GetSalePos()->GetPrintInterface();//	char *pt = m_szBuff;//aori del
	printer->nSetPrintBusiness(PBID_CHECK_STUB);
	int i=0,nMaxLen = GetSalePos()->GetParams()->m_nMaxPrintLineLen;
	if ( GetSalePos()->GetParams()->m_nPaperType == 1 )
		nMaxLen = 32;	//打印横线
	GetPrintChars(m_szBuff, nMaxLen, nMaxLen, true);
	printer->nPrintLine( m_szBuff );
	//打印抬头	
	sprintf(m_szBuff, "日期:%04d%02d%02d %02d:%02d收银员:%s", 
		m_stSaleDT.wYear, m_stSaleDT.wMonth, m_stSaleDT.wDay, m_stSaleDT.wHour, m_stSaleDT.wMinute, 
		GetSalePos()->GetCasher()->GetUserCode());
	printer->nPrintLine( m_szBuff );
	sprintf(m_szBuff, "机号:%03d 单号:%08d", 
			GetSalePos()->GetParams()->m_nPOSID,m_nSaleID);
	printer->nPrintLine( m_szBuff );


	//尾//总金额和总件数
	double fTotal=GetTotalAmt_beforePromotion();//aori change 2012-2-9 10:25:40 proj 2.79 测试反馈：条件促销时 应付金额 多扣 调用 GetTotalAmt_beforePromotion
	// ReducePrecision2(fTotal,GetSalePos()->GetParams()->m_nPayPrecision);
	//零钱包与会员的抹零控制
	if ( strlen(m_MemberImpl.m_Member.szMemberCode) > 1 )
	{
		if (GetSalePos()->GetParams()->m_bWallet_Wipe_Zero)   //使用零钱包是否抹零  0:不抹零， 1：抹零
		{
			ReducePrecision2(fTotal,GetSalePos()->GetParams()->m_nPayPrecision);//2:舍厘 1://舍分
		}
	
	}
	else
	{
		ReducePrecision2(fTotal,GetSalePos()->GetParams()->m_nPayPrecision);
	}


	sprintf(m_szBuff,"购买件数:%.0f  应付金额:%.2f",GetTotalCount(),fTotal);
	printer->nPrintLine( m_szBuff );

	const TPaymentStyle *ps = NULL;

	//打印支付方式及金额
	for ( vector<PayInfo>::const_iterator pay = m_PayImpl.m_vecPay.begin(); pay != m_PayImpl.m_vecPay.end(); ++pay) {
		vector<PayInfoItem> vecFindPayItem;
		m_PayImpl.GetPayInfoItemVect(pay->nPSID, &vecFindPayItem);

		ps = m_PayImpl.GetPaymentStyle(pay->nPSID);

		//strcpy(pt, m_PayImpl.GetPaymentStyle(pay->nPSID)->szDescription);
		//sprintf(m_szBuff + 200, "%12.2f", pay->fPayAmt);

		if ( NULL != ps )
		{
			sprintf(m_szBuff,"%s:%12.2f",ps->szDescription,pay->fPayAmt);
			printer->nPrintLine( m_szBuff);//pt );//aori del
		}	

		//thinpon. 打印支付号码
		if (NULL != ps && ps->bNeedPayNO )
		{
			sprintf(m_szBuff,"号码:%s",pay->szPayNum);//aori yinhang???
			if(pay->nPSID==GetSalePos()->GetParams()->m_nBankAppPSID ||
				pay->nPSID==GetSalePos()->GetParams()->m_nBankAuxPsid	)
			{
				CString cardnum(m_szBuff);
				for(int i=0;i<4;i++)
				{
					cardnum.SetAt(cardnum.GetLength()-1-4-i,'*');
				}
				strcpy(m_szBuff,cardnum.GetBuffer(cardnum.GetLength()+1));
				cardnum.ReleaseBuffer();
			}
			printer->nPrintLine(m_szBuff);//pt );//aori del
		}
	
		if(pay->nPSID == GetSalePos()->GetParams()->m_nSavingCardPSID)
		{
			bNeedSign = true;
			if (vecFindPayItem.size() > 0)
			{
				vector<PayInfoItem>::iterator payInfoItem = vecFindPayItem.begin(); 
				sprintf(m_szBuff,"余额:%.2f",payInfoItem->fRemainAmt);
				printer->nPrintLine(m_szBuff);
			}
		}
	}

	//打印找零额
	double dTemp = m_PayImpl.GetChange();
	ReducePrecision(dTemp);
	sprintf(m_szBuff, "实收金额:%9.2f 找零:%5.2f",m_PayImpl.GetPayAmt(),dTemp);
	//GetPrintFormatLines("找零", m_szBuff + 200, m_szBuff, nMaxLen);
	printer->nPrintLine(m_szBuff);

	printer->nPrintLine(" ");
	printer->nPrintLine(" ");
	printer->nPrintLine(" ");
	//printer->nSetPrintBusiness(PBID_TRANS_RECEIPT);
	if (bNeedSign)
	{
		sprintf(m_szBuff, "%s%s", "雇员签名:","....................");
		printer->nPrintLine(m_szBuff);
		printer->nPrintLine(" ");
		sprintf(m_szBuff, "%s%s", "顾客签名:","....................");
		printer->nPrintLine(m_szBuff);
		printer->nPrintLine(" ");
		printer->nPrintLine(" ");

		ps = NULL;
		for ( vector<PayInfo>::const_iterator pay = m_PayImpl.m_vecPay.begin(); pay != m_PayImpl.m_vecPay.end(); ++pay) {
			if (pay->nPSID != GetSalePos()->GetParams()->m_nSavingCardPSID)
				continue;
			vector<PayInfoItem> vecFindPayItem;
			m_PayImpl.GetPayInfoItemVect(pay->nPSID, &vecFindPayItem);
			
			ps = m_PayImpl.GetPaymentStyle(pay->nPSID);
			if ( NULL != ps )
			{
				sprintf(m_szBuff,"%s:%12.2f",ps->szDescription,pay->fPayAmt);
				printer->nPrintLine( m_szBuff);
			}			
			
			//打印支付号码
			if ( NULL != ps && ps->bNeedPayNO)
			{
				sprintf(m_szBuff,"号码:%s",pay->szPayNum);
				printer->nPrintLine(m_szBuff);
			}
			if (vecFindPayItem.size() > 0)
			{
				vector<PayInfoItem>::iterator payInfoItem = vecFindPayItem.begin(); 
				sprintf(m_szBuff,"余额:%.2f",payInfoItem->fRemainAmt);
				printer->nPrintLine(m_szBuff);
			}
		}
		printer->nPrintLine(" ");
		sprintf(m_szBuff, "%s%s", "雇员签名:","....................");
		printer->nPrintLine(m_szBuff);
		printer->nPrintLine(" ");
		sprintf(m_szBuff, "%s%s", "顾客签名:","....................");
		printer->nPrintLine(m_szBuff);
		printer->nPrintLine(" ");
		printer->nPrintLine(" ");
	}
}


//======================================================================
// 函 数 名：PrintBillWHole
// 功能描述：打印
// 输入参数：
// 输出参数：void
// 创建日期：2009-4-6 
// 修改日期：
// 作  者：曾海林
// 附加说明：//
//======================================================================
void CSaleImpl::PrintBillWHole(ePrintPart ePart)
{
	//打印机不可用则退出
	if (!GetSalePos()->GetPrintInterface()->m_bValid) 
	{
		GetSalePos()->GetWriteLog()->WriteLog("END PrintBillWhole PrintInterface::m_bValid禁止打印",4,POSLOG_LEVEL1); 
		return;
	} 

	if(!GetSalePos()->GetPrintInterface()->GetPOSPrintConfig(PBID_TRANS_RECEIPT).Enable)
	{
		GetSalePos()->GetWriteLog()->WriteLog("END PrintBillWhole PrintInterface::打印配置为不打印交易小票",4,POSLOG_LEVEL1); 
		return;
	}

	if(m_ServiceImpl.HasSVMerch())
		GetSalePos()->GetPrintInterface()->nSetPrintBusiness(PBID_SERVICE_RECEIPT);
	else
		GetSalePos()->GetPrintInterface()->nSetPrintBusiness(PBID_TRANS_RECEIPT);

	//打印服饰小票
 	if (GetSalePos()->GetParams()->m_bPoolClothesMerch)
 	{
 		//完整小票
 		GetSalePos()->GetPrintInterface()->nPrintLine("---------- 收款员联 -----------");
 		
 		/************************************************************************/
 		/* Modified by dandan.wu 2015-12-29*/
 		/*若按下了预打印小票按钮，则只需打印后面部分；否则，全打印*/
 		if ( ePart == PRINT_BEFORE )
 		{
 			printBillBeforeCheckOut();
 		}
 		else if ( ePart == PRINT_AFTER )
 		{
 			printBillAfterCheckOut();
 		}
 		else if ( ePart == PRINT_WHOLE )
 		{
 			PrintBillWhole_ONE();
 		}
		else if ( ePart == PRINT_CANCEL )
		{
			printBillAboveCancel();
		}
		else
		{
			CUniTrace::Trace(_T("无效的打印！"));
		}
 		/************************************************************************/
 
 		//各联营大码小票
 		map<CString, double> mapPoolClothesMerch;
 		GetPoolClothesMerch(&m_vecSaleMerch,&mapPoolClothesMerch);
 		int count = 1;
 		map<CString, double>::iterator itr; 
 		for (itr = mapPoolClothesMerch.begin(); itr != mapPoolClothesMerch.end(); itr++, count++)
 		{	
 			GetSalePos()->GetPrintInterface()->nPrintLine(" ");
 			sprintf(m_szBuff,"---营业员联:(共 %d 联) 第 %d 联---",mapPoolClothesMerch.size(), count);
 			GetSalePos()->GetPrintInterface()->nPrintLine(m_szBuff);
 			CString poolSKU=itr->first;
 			sprintf(m_szBuff," 联营大码: %s ",poolSKU);
 			GetSalePos()->GetPrintInterface()->nPrintLine(m_szBuff);
 			GetSalePos()->GetPrintInterface()->nPrintHorLine();
 
 			//获取联营大码对应商品
 			vector<SaleMerchInfo> vecSaleMerch_pool;
 			for ( vector<SaleMerchInfo>::iterator merchInfo = m_vecSaleMerch.begin(); merchInfo != m_vecSaleMerch.end(); ++merchInfo) 
 			{
 				if(merchInfo->nMerchState != RS_CANCEL)
 				{
 					CString curSKU=GetPoolSKU(merchInfo->szSimpleName);
 					if(strcmp(poolSKU,curSKU)==0)
 						vecSaleMerch_pool.push_back(*merchInfo);
 				}
 			}
 			//打印
 			PrintBillWhole_ADD(&vecSaleMerch_pool);
 			GetSalePos()->GetPrintInterface()->nPrintLine(" ");
 		}
 		for(int i = 0; i<7; i++)
 		{
 			GetSalePos()->GetPrintInterface()->nPrintLine("     ");
 		}
 	}
 	else
	{
		/************************************************************************/
		/* Modified by dandan.wu 2015-12-29*/
	    /*若按下了预打印小票按钮，则只需打印后面部分；否则，全打印*/
		if ( ePart == PRINT_BEFORE )
		{
			printBillBeforeCheckOut();
		}
		else if ( ePart == PRINT_AFTER )
		{
			printBillAfterCheckOut();
		}
		else if ( ePart == PRINT_WHOLE )
		{
			PrintBillWhole_ONE();
		}
		else if ( ePart == PRINT_CANCEL )
		{
			printBillAboveCancel();
		}
		else
		{
			CUniTrace::Trace(_T("无效的打印！"));
		}
		/************************************************************************/
	}
}
// 
//打印完整小票
void CSaleImpl::PrintBillWhole_ONE()
{
	GetSalePos()->GetWriteLog()->WriteLog("BEGIN PrintBillWhole_ONE",4,POSLOG_LEVEL1); 

	GetSalePos()->GetWriteLog()->WriteLog("开始打印小票",2);////记录日志---------------------
	
	//打印小票头&商品行（非逐行打印）
	if (GetSalePos()->m_params.m_nPrinterType==1)
	{
		SortSaleMerch();
		PrintBillHeadSmall();//小票头
		m_bReprint?PrintBillLine(g_STRREPRINT):1;//商品行//int nMerchNum=0;//aori del
//		for (vector<SaleMerchInfo>::iterator merchInfo = m_vecSaleMerch.begin(); merchInfo != m_vecSaleMerch.end(); ++merchInfo) 
//		{
//			if (merchInfo->nMerchState != RS_CANCEL)
//			{
//				double fsalequantity=0.0f,factualprice=0.0f;
//				merchInfo->OutPut_Quantity_Price(fsalequantity,factualprice);
//			 	PrintBillItemSmall(merchInfo,fsalequantity,factualprice); //aori optimize 2011-10-13 14:53:19 逐行打印replace//printbillitemsmall(merchinfo->szplu,szname,fsalequantity,factualprice,merchinfo->fsaleamt); //cstring szname;szname.format("%s%s",merchinfo->npromoid>0?"(促)":"",merchinfo->szsimplename);//bug：2011-2-24 10:52:36促销打印用promoid 而不用salepromoid判断。
//			 }
//		}

		//打印现货商品明细
		printBillItemInStock(m_vecSaleMerch);
		
		//打印订单明细
		printBillItemInOrder(m_vecSaleMerch);
	}

	m_bReprint?PrintBillLine(g_STRREPRINT):1;	

	//横线
	GetSalePos()->GetPrintInterface()->nPrintHorLine(); 	

	//打印合计
    PrintBillTotalSmall();

	//打印联营服饰商品大码合计
	if (GetSalePos()->GetParams()->m_bPoolClothesMerch) 
		PrintPoolClothesMerch(); 

	m_bReprint?PrintBillLine(g_STRREPRINT):1;

	//没执行
	//PrintBillPromoStrInfo();	//打印参与条件促销的商品

	//单品印花，百安居没有此需求，不打印[dandan.wu 2016-3-25]
	//GetSalePos()->GetPrintInterface()->PrintYinHua(m_vecSaleMerch);

	//整单印花，百安居没有此需求，不打印[dandan.wu 2016-3-25]
	//m_strategy_WholeBill_Stamp->print_wholebill_stamp();		   

	//税控发票 
	if (GetSalePos()->GetParams()->m_nPrintTaxInfo == 1)
	{
		GetSalePos()->GetWriteLog()->WriteLog("开始税控发票打印",3); 
		CTaxInfoImpl *TaxInfoImpl=GetSalePos()->GetTaxInfoImpl();
		if(m_ServiceImpl.HasSVMerch())
			TaxInfoImpl->PrintInvInfo(PBID_SERVICE_RECEIPT);
		else
			TaxInfoImpl->PrintInvInfo(PBID_TRANS_RECEIPT);
		GetSalePos()->GetWriteLog()->WriteLog("结束税控发票打印",3); 
	} 

	//打印票尾
	PrintBillBootomSmall();

	GetSalePos()->GetWriteLog()->WriteLog("END PrintBillWhole_ONE",4,POSLOG_LEVEL1);//aori move from checkoutdlg::oncheckout 2011-8-30 10:23:53 proj2011-8-30-2 优化可读性
	//GetSalePos()->GetPrintInterface()->nSetPrintBusiness(PBID_TRANS_RECEIPT);
}

void CSaleImpl::printBillItemInStock(vector<SaleMerchInfo>& vecSaleMerch)
{
	//打印机不可用则退出
	if (!GetSalePos()->GetPrintInterface()->m_bValid) 
	{
		CUniTrace::Trace(_T("打印现货商品：打印机不可用"));
		return;
	}

	//int nTypeInStock = 1;							//标记类型为现货商品
	int nCount = 0;									//标记现货商品的总数
	char szBuff[256]={0};
	double dbSumInStock = 0.0f;
	double dbSumReal = 0.0f;
	double fSaleQuantity=0.0f,fActualPrice=0.0f;
	vector<SaleMerchInfo>::iterator merchInfo;
	bool bInvoicePrint = false;
	bInvoicePrint = GetSalePos()->m_params.m_bAllowInvoicePrint;
	
	for (merchInfo = vecSaleMerch.begin(); merchInfo != vecSaleMerch.end(); ++merchInfo) 
	{
		//被取消商品不算	
		if (merchInfo->nMerchState == RS_CANCEL)
		{
			continue;
		}
		
		if ( merchInfo->nItemType == ITEM_IN_STOCK )
		{
			nCount++;
		}
	}
	
	if ( nCount <= 0 )
	{
		CUniTrace::Trace(_T("没有现货商品需要打印"));
		return;
 	}

	//打印空行
	if (!bInvoicePrint)
	{
		GetSalePos()->GetPrintInterface()->nPrintHorLine(' ');
	}
		
	//打印标题:现货商品
	memset(szBuff,0,sizeof(szBuff));
	PrintBillLine(SALEITEM_TITLE_STOCK,DT_LEFT);
	
	//打印横线
	if (!bInvoicePrint)
	{
		GetSalePos()->GetPrintInterface()->nPrintHorLine();
	}
	
	//打印明细
	for (merchInfo = vecSaleMerch.begin(); merchInfo != vecSaleMerch.end(); ++merchInfo) 
	{
		if (merchInfo->nMerchState != RS_CANCEL && merchInfo->nItemType == ITEM_IN_STOCK )
		{
			merchInfo->OutPut_Quantity_Price(fSaleQuantity,fActualPrice);

			PrintBillItemSmall(merchInfo,fSaleQuantity,fActualPrice); //aori optimize 2011-10-13 14:53:19 逐行打印replace//PrintBillItemSmall(merchInfo->szPLU,szName,fSaleQuantity,fActualPrice,merchInfo->fSaleAmt); //CString szName;szName.Format("%s%s",merchInfo->nPromoID>0?"(促)":"",merchInfo->szSimpleName);//bug：2011-2-24 10:52:36促销打印用promoid 而不用salepromoid判断。
		
			dbSumInStock += fSaleQuantity*fActualPrice;//merchInfo->fSaleAmt_BeforePromotion;
			dbSumReal += merchInfo->fSaleAmt;
		}
	}	

	//打印合计
	if ( !bInvoicePrint )
	{
		GetSalePos()->GetPrintInterface()->nPrintTotal_m(dbSumInStock,dbSumReal,PBID_TRANS_RECEIPT);
	}
}

void CSaleImpl::printBillItemInOrder(vector<SaleMerchInfo>& vecSaleMerch)
{
	//打印机不可用则退出
	if (!GetSalePos()->GetPrintInterface()->m_bValid) 
	{
		CUniTrace::Trace(_T("打印订单商品：打印机不可用"));
		return;
	}

	//int nType = 2;								//标记类型为订单商品
	int nCount = 0;									//标记现货商品的总数
	char m_szBuff[256]={0};
	double dbSum = 0.0f,dbSumReal = 0.0f;			//合计
	double fSaleQuantity=0.0f,fActualPrice=0.0f;
	vector<CString> vecStrOrderNo;					//订单号是一串数字，没有字母
	vector<SaleMerchInfo>::iterator merchInfo;
	vector<CString>::iterator itOrderNo;
	bool bInvoicePrint = false;
	bInvoicePrint = GetSalePos()->m_params.m_bAllowInvoicePrint;
	
	//保存所有订单号，不能重复
	for ( merchInfo = vecSaleMerch.begin(); merchInfo != vecSaleMerch.end(); ++merchInfo) 
	{
		//被取消商品不算	
		if (merchInfo->nMerchState == RS_CANCEL)
		{
			continue;
		}

		//若类型为订单商品，一定有订单号
		if ( merchInfo->nItemType == ITEM_IN_ORDER && !merchInfo->strOrderNo.IsEmpty() )
		{
			vecStrOrderNo.push_back(merchInfo->strOrderNo);		
		}
	}	

	int n = 0;

	//订单号去重
	if ( vecStrOrderNo.size() > 1 )
	{
		sort(vecStrOrderNo.begin(), vecStrOrderNo.end()); 
		
		n = vecStrOrderNo.size();
		itOrderNo = unique(vecStrOrderNo.begin(),vecStrOrderNo.end());	
		vecStrOrderNo.erase( itOrderNo, vecStrOrderNo.end());

		n = vecStrOrderNo.size();
	}
	
	for ( itOrderNo = vecStrOrderNo.begin();itOrderNo != vecStrOrderNo.end(); ++itOrderNo )
	{
		dbSum = 0.0f;
		dbSumReal = 0.0f;

		if ( !bInvoicePrint )
		{
			GetSalePos()->GetPrintInterface()->nPrintHorLine(' '); 
		}

		//打印标题:销售订单：xxx	
		memset(m_szBuff,0,sizeof(m_szBuff));
		sprintf(m_szBuff, "%s:%s", SALEITEM_TITLE_ORDER,*itOrderNo);
		PrintBillLine(m_szBuff,DT_LEFT);

		//打印横线
		if ( !bInvoicePrint )
		{
			GetSalePos()->GetPrintInterface()->nPrintHorLine(); 
		}
		
		//打印明细
		for (merchInfo = vecSaleMerch.begin(); merchInfo != vecSaleMerch.end(); ++merchInfo) 
		{
			if ( merchInfo->nMerchState != RS_CANCEL 
				 && merchInfo->nItemType == ITEM_IN_ORDER 
				 && itOrderNo->Compare(merchInfo->strOrderNo) == 0 )
			{
				merchInfo->OutPut_Quantity_Price(fSaleQuantity,fActualPrice);

				PrintBillItemSmall(merchInfo,fSaleQuantity,fActualPrice); //aori optimize 2011-10-13 14:53:19 逐行打印replace//PrintBillItemSmall(merchInfo->szPLU,szName,fSaleQuantity,fActualPrice,merchInfo->fSaleAmt); //CString szName;szName.Format("%s%s",merchInfo->nPromoID>0?"(促)":"",merchInfo->szSimpleName);//bug：2011-2-24 10:52:36促销打印用promoid 而不用salepromoid判断。
	
				//MarkUp只打印合计 [dandan.wu 2017-11-1]
				if ( merchInfo->fManualDiscount < 0 )
				{
					dbSum += merchInfo->fSaleAmt_BeforePromotion;
					dbSumReal += merchInfo->fSaleAmt_BeforePromotion;
				}
				else
				{
					dbSum += fSaleQuantity*fActualPrice;//merchInfo->fSaleAmt_BeforePromotion;					
					dbSumReal += merchInfo->fSaleAmt;
				}			
			}
		}

		//打印合计
		if ( !bInvoicePrint )
		{
			GetSalePos()->GetPrintInterface()->nPrintTotal_m(dbSum,dbSumReal,PBID_TRANS_RECEIPT);		
		}
	}	
}

void CSaleImpl::printBillBeforeCheckOut()
{
	//打印小票头&商品行（非逐行打印）
	if (GetSalePos()->m_params.m_nPrinterType==1)
	{
		GetSalePos()->GetWriteLog()->WriteLog("Begin to call CSaleImpl::printBillBeforeCheckOut()",2);

		SortSaleMerch();

		//小票头
		PrintBillHeadSmall();

		m_bReprint?PrintBillLine(g_STRREPRINT):1;//商品行//int nMerchNum=0;//aori del
	
		//打印现货商品明细
		printBillItemInStock(m_vecSaleMerchPrePrint);

		//打印订单明细
		printBillItemInOrder(m_vecSaleMerchPrePrint);

		GetSalePos()->GetWriteLog()->WriteLog("End to call CSaleImpl::printBillBeforeCheckOut()",2);
	}
}

void CSaleImpl::printBillAfterCheckOut()
{
	GetSalePos()->GetWriteLog()->WriteLog("Begin to call CSaleImpl::printBillAfterCheckOut()...",2);

	m_bReprint?PrintBillLine(g_STRREPRINT):1;	
	
	//横线
	GetSalePos()->GetPrintInterface()->nPrintHorLine(); 	
	
	//打印合计
    PrintBillTotalSmall();
	
	//打印联营服饰商品大码合计
	if (GetSalePos()->GetParams()->m_bPoolClothesMerch) 
		PrintPoolClothesMerch(); 
	
	m_bReprint?PrintBillLine(g_STRREPRINT):1;
	
	//没执行
	//PrintBillPromoStrInfo();	//打印参与条件促销的商品
	
	//单品印花
	//GetSalePos()->GetPrintInterface()->PrintYinHua(m_vecSaleMerch);
	
	//整单印花
	//m_strategy_WholeBill_Stamp->print_wholebill_stamp();		   
	
	//税控发票 
	if (GetSalePos()->GetParams()->m_nPrintTaxInfo == 1)
	{
		GetSalePos()->GetWriteLog()->WriteLog("开始税控发票打印",3); 
		CTaxInfoImpl *TaxInfoImpl=GetSalePos()->GetTaxInfoImpl();
		if(m_ServiceImpl.HasSVMerch())
			TaxInfoImpl->PrintInvInfo(PBID_SERVICE_RECEIPT);
		else
			TaxInfoImpl->PrintInvInfo(PBID_TRANS_RECEIPT);
		GetSalePos()->GetWriteLog()->WriteLog("结束税控发票打印",3); 
	} 
	
	//打印票尾
	PrintBillBootomSmall();

	GetSalePos()->GetWriteLog()->WriteLog("End to call CSaleImpl::printBillAfterCheckOut()...",2);
}

void CSaleImpl::printBillAboveCancel()
{
	//PrintBillLine("-----------以上作废-----------");
	GetSalePos()->GetPrintInterface()->nPrintAboveCancel_m(PBID_TRANS_RECEIPT);
}

void CSaleImpl::printBillSumDiscount()
{
	//打印机不可用则退出
	if (!GetSalePos()->GetPrintInterface()->m_bValid) 
	{
		CUniTrace::Trace(_T("打印卡折扣：打印机不可用"));
		return;
	}
	
	double dbSumDis = 0.0f;				//总优惠

	double dbSumCondPromDis = 0,dbManualDiscount = 0,dbMemDiscAfterProm = 0;
	double dbSumAmt = 0;

	for ( vector<SaleMerchInfo>::iterator merchInfo = m_vecSaleMerch.begin(); merchInfo != m_vecSaleMerch.end(); ++merchInfo )
	{
		if ( merchInfo->nMerchState != RS_CANCEL ) 
		{
			dbSumAmt += merchInfo->fSaleAmt;
			dbSumCondPromDis += merchInfo->dCondPromDiscount;//条件促销
			
			//只打印MarkDown折扣，因为为了顾客体验，MarkUp商品只打印一行: PLU MarkUp后的单价 总价[dandan.wu 2017-11-1]
			if ( merchInfo->fManualDiscount > 0 )
			{
				dbManualDiscount += merchInfo->fManualDiscount;
			}

			dbMemDiscAfterProm += merchInfo->dMemDiscAfterProm;//会员
		}
	}
	
	dbSumDis  = dbSumCondPromDis + dbMemDiscAfterProm+dbManualDiscount;
	ReducePrecision(dbMemDiscAfterProm);
	ReducePrecision(dbSumDis);

	if ( dbSumDis == 0 )
	{
		return;
	}
		
	GetSalePos()->GetPrintInterface()->nPrintMemDiscount_m(-dbMemDiscAfterProm,-dbManualDiscount,-dbSumDis,PBID_TRANS_RECEIPT);
}


//打印追加小票(服饰系统专用)
void CSaleImpl::PrintBillWhole_ADD(vector<SaleMerchInfo>* pVecSaleMerch)
{
	GetSalePos()->GetWriteLog()->WriteLog("BEGIN PrintBillWhole_ADD",4,POSLOG_LEVEL1); 
	GetSalePos()->GetWriteLog()->WriteLog("开始打印服饰小票",2);////记录日志---------------------	
	//打印小票头&商品行（非逐行打印）
	//SortSaleMerch();
	PrintBillHeadSmall();//小票头
	m_bReprint?PrintBillLine(g_STRREPRINT):1;//商品行//int nMerchNum=0;//aori del
	for (vector<SaleMerchInfo>::iterator merchInfo = pVecSaleMerch->begin(); merchInfo != pVecSaleMerch->end(); ++merchInfo) 
	{
		if (merchInfo->nMerchState != RS_CANCEL)
		{
			double fSaleQuantity=0.0f,fActualPrice=0.0f;
			merchInfo->OutPut_Quantity_Price(fSaleQuantity,fActualPrice);
			PrintBillItemSmall(merchInfo,fSaleQuantity,fActualPrice); 
		}
	}
	m_bReprint?PrintBillLine(g_STRREPRINT):1;	

	GetSalePos()->GetPrintInterface()->nPrintHorLine(); 	//横线
    PrintBillTotalSmall(pVecSaleMerch,true);	 //打印合计
	if (GetSalePos()->GetParams()->m_bPoolClothesMerch,true) //打印联营服饰商品大码合计
		PrintPoolClothesMerch(pVecSaleMerch); 
	m_bReprint?PrintBillLine(g_STRREPRINT):1;
	GetSalePos()->GetWriteLog()->WriteLog("END PrintBillWhole_ADD",4,POSLOG_LEVEL1);//aori move from checkoutdlg::oncheckout 2011-8-30 10:23:53 proj2011-8-30-2 优化可读性
}

//写销售日志 zenghl 20090608
//参数说明theAll=1 表示整个小票写入日志

int CSaleImpl::WriteSaleLog(bool theAll)
{	
    //判断是否需要记录销售明细记录
	if (GetSalePos()->GetParams()->m_nRecordSaleItem!=1) return -1;//不记录销售明细记录
	int Count=m_vecSaleMerch.size(); 
	SaleMerchInfo newMerchInfo("");
	if (theAll)//整个小票
	{
		if (Count>0)//写除最后一笔以外的所有条件售明细
		{
			for (int i=0;i<Count;i++)
			{
				newMerchInfo=m_vecSaleMerch.at(i);
				if (newMerchInfo.nMerchState == RS_CANCEL) continue;//单品取消不记录
				WriteSaleItemLog(newMerchInfo,0,3);
			}
		}	
	}
	else if (Count>0)//写最后一条销售明细
	{
		newMerchInfo=m_vecSaleMerch.at(Count-1);
		WriteSaleItemLog(newMerchInfo,0,4);
	}

return 1;
}
//写销售明细日志 zenghl 20090608
//参数说明 Cancel=1 表示取消结算取消，表示取消某笔结算
int CSaleImpl::WriteSaleItemLog(SaleMerchInfo &newMerchInfo, bool Cancel,int callflag)
{//aori add 2011-5-13 10:53:09 proj2011-5-11-2：销售不平5-9北科大3: 日志级别 writelog写成dll
	CString strLogBuf, hehe;
	switch(callflag){//aori add 2011-5-11 23:45:00 proj2011-5-11-2：销售不平5-9北科大
		case 0:hehe.Format("cancel:");
			break;
		case 1:hehe.Format("线程%ld 商品明细 Modify_Quantity:",GetCurrentThreadId());
			break;//aori add 2012-3-14 14:20:47  proj 2012-3-5 消息机制bug 2.加mutex 于 OnInputNumber 和 OnCheckOut	
		case 2:hehe.Format("商品明细 Modify_Price:");
			break;
		case 3:hehe.Format("unHangUp the_all:");
			break;
		case 4:hehe.Format("unHangUp the_last:");
			break;
		case 5:hehe.Format("check_Limit:");
			break;
		case 6:hehe.Format("handle_Limit:");
			break;
	}
	if (Cancel){//取消操作动作记录负值
		if (newMerchInfo.nManagementStyle==2){//金额类商品
			if (newMerchInfo.bFixedPriceAndQuantity)//18码
			{
				strLogBuf.Format("%s %s  %4f %4f %2f",newMerchInfo.szPLU,newMerchInfo.szPLUInput,newMerchInfo.fActualPrice,-newMerchInfo.fSaleQuantity,-newMerchInfo.fSaleAmt);
			}
			else if(newMerchInfo.bPriceInBarcode)//13码
			{
				strLogBuf.Format("%s %s  %4f %d %2f",newMerchInfo.szPLU,newMerchInfo.szPLUInput,newMerchInfo.fActualPrice,-newMerchInfo.nSaleCount,-newMerchInfo.fSaleAmt);
			}else//大码
			{
				strLogBuf.Format("%s %s  %4f %d %2f",newMerchInfo.szPLU,newMerchInfo.szPLUInput,newMerchInfo.fActualPrice,-newMerchInfo.nSaleCount,-newMerchInfo.fActualPrice);
			}
		}
		else
		{
			strLogBuf.Format("%s %s  %.4f %d %.2f",newMerchInfo.szPLU,newMerchInfo.szPLUInput,newMerchInfo.fActualPrice,-newMerchInfo.nSaleCount,-newMerchInfo.fActualPrice*newMerchInfo.nSaleCount);
		}
	}else{//非取消操作
		
		if (newMerchInfo.nManagementStyle==2)//金额类商品
		{
			
			if (newMerchInfo.bFixedPriceAndQuantity)//18码
			{
				strLogBuf.Format("%s %s  %4f %4f %2f",newMerchInfo.szPLU,newMerchInfo.szPLUInput,newMerchInfo.fActualPrice,newMerchInfo.fSaleQuantity,newMerchInfo.fSaleAmt);
			}
			else if(newMerchInfo.bPriceInBarcode)//13码
			{
				strLogBuf.Format("%s %s  %4f %d %2f",newMerchInfo.szPLU,newMerchInfo.szPLUInput,newMerchInfo.fActualPrice,newMerchInfo.nSaleCount,newMerchInfo.fSaleAmt);
			}else//大码
			{
				strLogBuf.Format("%s %s  %4f %d %2f",newMerchInfo.szPLU,newMerchInfo.szPLUInput,newMerchInfo.fActualPrice,newMerchInfo.nSaleCount,newMerchInfo.fActualPrice);
			}
		}
		else
		{
			strLogBuf.Format("%s %s  %.4f %d %.2f",newMerchInfo.szPLU,newMerchInfo.szPLUInput,newMerchInfo.fActualPrice,newMerchInfo.nSaleCount,newMerchInfo.fActualPrice*newMerchInfo.nSaleCount);
		}
	}
	//hehe+=strLogBuf;
	strLogBuf = hehe + strLogBuf;
	GetSalePos()->GetWriteLog()->WriteLog(strLogBuf,3,2);//写销售日志
	return 1;
}

/*
int CSaleImpl::PasswdVaildDay()
{

	CPosClientDB *pDB= CDataBaseManager::getConnection();

	if ( !pDB ){
	GetSalePos()->GetWriteLog()->WriteLog("连接posdb失败",4);
	return false;
	}

	m_npasswdvaliddays=0;
		CString strSQL;
		strSQL.Format("select isnull(datediff(day,getdate(),PwdValidDate),10000) from cashier where usercode = '%s' " ,
			GetSalePos()->GetCasher()->GetUserCode() );
			CRecordset* pRecord = NULL;
			CDBVariant var;
			try {
				if ( pDB->Execute(strSQL, &pRecord) > 0 ) {
					pRecord->GetFieldValue((short)0, var);
					//if ( var.m_lVal > 0 )
					if (pRecord->GetRecordCount() > 0){
						m_npasswdvaliddays=var.m_iVal;
						//UserPwdValidDays=atoi(*var.m_pstring);
						GetSalePos()->GetWriteLog()->WriteLog("取密码更新日期成功",4);

					}
										
					pDB->CloseRecordset(pRecord);
				}
			} catch (CException* e) {
				CUniTrace::Trace("changepasswd::UserPwdValidDays");
				CUniTrace::Trace(*e);
				e->Delete();

			}

 
				CDataBaseManager::closeConnection(pDB);
              	return true;



}
*/

/*
int CSaleImpl::Getpasswdvaliddays()
{
  return m_npasswdvaliddays;
}*/

/*
bool CSaleImpl::Checkbonussales()
{
	GetSalePos()->GetWriteLog()->WriteLog("开始判断红利是否已经存在......",4);
	CPosClientDB *pDB= CDataBaseManager::getConnection(true);
    Checkbonusexist=false;

	if ( !pDB )
	return false;


	CString strSQL;
	CDBVariant var;
	CRecordset* pRecord = NULL;


	int posno=GetSalePos()->GetParams()->m_nPOSID;
	//CString ccc=m_MemberImpl.m_Member.szMemberCode;
    
	CString c_saledate=::GetFormatDTString(GetSaleDT());
	char *saledate = (char*)(LPCSTR)c_saledate;
	//salesno 销售编号 yyyymmdd0001000010 (yyyymmdd+pos4位+小票5位+0)
    CString salesno = c_saledate.Left(4);//year
	salesno=salesno+c_saledate.Mid(5,2);//month
	salesno=salesno+c_saledate.Mid(8,2);//day

	int rtn = 0;

	try {
		strSQL.Format("select count(*) from tIntCardPayFlow where convert(int,substring(saleno,13,5)) = %d and  posno = %d and  cardfaceno = '%s' and substring(saleno,1,8) = '%s' " ,m_nSaleID,posno,m_MemberImpl.m_Member.szMemberCode,salesno);

		if ( pDB->Execute(strSQL, &pRecord ) > 0 ) {
			pRecord->GetFieldValue( (short)0, var);
				rtn = var.m_iVal;
            if ( rtn != 0) {
			Checkbonusexist=true;
			}
		}
			pDB->CloseRecordset( pRecord );
	}
	catch(CException *e) 
	{
		pDB->CloseRecordset( pRecord );
		CUniTrace::Trace(*e);
		e->Delete();
	}

	CDataBaseManager::closeConnection(pDB);
	GetSalePos()->GetWriteLog()->WriteLog("判断红利是否已经存在结束.",4);
	return 1;
}
*/
 /*
bool CSaleImpl::Getbonusvalid()
{
   return Checkbonusexist;
}*/
 
bool CSaleImpl::SaveHangup(CPosClientDB *pDB, int nSaleID, unsigned char nSaleStatus)
{
	vector<SaleMerchInfo>::iterator merchInfo;

	double fTotDiscount = 0.0f;
	double fTotAmt = 0.0f, dTemp = 0.0, fTempMemdiscount = 0;
	CString strSQL, strManualDiscount;
	const int nUserID = GetSalePos()->GetCasher()->GetUserID();
	//修改24小时销售的问题，将saledt修改为开票的时间
//	CString strSaleDT = ::GetFormatDTString(m_stSaleDT);
	CString strSaleDT = ::GetFormatDTString(m_stEndSaleDT);
	const char *szTableDTExt = GetSalePos()->GetTableDTExt(&m_stEndSaleDT);
	double dManualDiscount = 0;

	//保存数据
	try	{
		/*fTotDiscount=GetSalePos()->GetPromotionInterface()->m_fDisCountAmt;
		//20090526 修改为每次保存数据时调用LoadSaleInfo(
		GetSalePos()->GetPromotionInterface()->SetSaleImpl(this);
		GetSalePos()->GetPromotionInterface()->LoadSaleInfo(nSaleID,true);//保存促销结果
		*/
	   //ReSortSaleMerch(); zenghl 20090528
		//保存到销售主表
		//计算销售折扣总计
		for ( merchInfo = m_vecSaleMerch.begin(); merchInfo != m_vecSaleMerch.end(); ++merchInfo ) {
			if ( merchInfo->nMerchState != RS_CANCEL ) {
				//fTotDiscount += merchInfo->fSaleDiscount;//以后tot里面不加会员的了 + merchInfo->fMemSaleDiscount;
				fTotAmt += merchInfo->fSaleAmt;
				fTempMemdiscount += merchInfo->fMemSaleDiscount;
				dManualDiscount += merchInfo->fManualDiscount;
			}
		}
		string strCC;
		if ( m_strCustomerCharacter.size() > 0 ) {
			strCC = "'";
			strCC += m_strCustomerCharacter;
			strCC += "'";
		} else
			strCC = "NULL";

		CTime tmSaleDT(m_stSaleDT), tmEndSaleDT(m_stEndSaleDT), tmCheckSaleDT(m_stCheckSaleDT);
		int nEndSaleSpan = (tmEndSaleDT - tmSaleDT).GetTotalSeconds();
		int nCheckSaleSpan = (tmCheckSaleDT - tmSaleDT).GetTotalSeconds();
		CString strAddCol6 = "";
		CString strMemCode = "";
		strMemCode.Format("%s", m_MemberImpl.m_Member.szInputNO);
		if(!strMemCode.IsEmpty())
		{
			strAddCol6.Format("%d=%s", m_MemberImpl.m_Member.nHumanID, m_MemberImpl.m_Member.szMemberName);
			//30位截断
			if (strAddCol6.GetLength() > 30)
			{
				int i = 0;
				for(;i < 28;)	
				{
					unsigned char c = strAddCol6[i];
					if(strAddCol6[i] & 0x80)
					{
						i+=2;
					}
					else
					{
						i++;
					}
					if(i >= 28)
						break;
				}		
				strAddCol6 = strAddCol6.Left(i);
			}
		}
		//折扣取整到分
		//ReducePrecision(fTotDiscount);
		//会员input
		//cardtype=0;
		//MemberCode = "";
		//Get_Member_input();
// 
// 	strSQL.Format("INSERT INTO Sale%s(SaleDT,EndSaleDT,CheckSaleDT,SaleID,HumanID,TotAmt,TotDiscount,Status,PDID,MemberCode,CharCode,ManualDiscount,MemDiscount,UploadFlag,MemIntegral,Shift_No,MemCardNO,MemCardNOType) "
// 				"VALUES ('%s',%d,%d,%ld,%ld,%.4f,%.4f,%d,%d,'%s',%s,%.4f,%.4f,0,%f,%d ,'%s',%d)", 
// 				szTableDTExt, strSaleDT, nEndSaleSpan, nCheckSaleSpan, 
// 				nSaleID, nUserID, fTotAmt, fTotDiscount, 
// 				nSaleStatus, GetSalePos()->GetCurrentTimePeriod(), m_MemberImpl.m_Member.szMemberCode,//GetFieldStringOrNULL(m_MemberImpl.m_Member.szMemberCode), 
// 				strCC.c_str(), m_fDiscount, fTempMemdiscount,m_MemberImpl.m_dMemIntegral,m_uShiftNo ,m_MemberImpl.m_Member.szInputNO,m_MemberImpl.m_Member.nCardNoType);//把会员的整单打折也算到里面去m_fMemDiscount);
		strSQL.Format("INSERT INTO Sale%s(SaleDT,EndSaleDT,CheckSaleDT,SaleID,HumanID,TotAmt,TotDiscount,Status,PDID,MemberCode,CharCode,ManualDiscount,MemDiscount,UploadFlag,MemIntegral,Shift_No,stampcount,addcol5,MemCardNO,MemCardNOType,AddCol6,AddCol7,AddCol8," 
			" MemberCardType,Ecardno,userID,MemberCardLevel,MemberCardchannel,PromoterID  ) "//★★★ 2012-9-14 16:07:35 proj 2012-9-10 R2POS T1987 总单印花 保存
			"VALUES ('%s',%d,%d,%ld,%ld,%.4f,%.4f,%d,%d,'%s',%s,%.4f,%.4f,0,%f,%d,%d,%d ,'%s',%d,'%s','%s','%d' ,'%s','%s','%s','%s','%s','%s')", 
			szTableDTExt, strSaleDT, nEndSaleSpan, nCheckSaleSpan, 
			nSaleID, nUserID, fTotAmt, fTotDiscount, 
			nSaleStatus, GetSalePos()->GetCurrentTimePeriod(),  m_MemberImpl.m_Member.szMemberCode,//GetFieldStringOrNULL(m_MemberImpl.m_Member.szMemberCode), 
			strCC.c_str(), /*m_fDiscount*/dManualDiscount, fTempMemdiscount,m_MemberImpl.m_dMemIntegral,m_uShiftNo,
			/*m_strategy_WholeBill_Stamp->m_YinHuaCount*/0,//百安居没有这个业务，这个字段应该填0，否则溢出[dandan.wu 2016-3-10]
			m_strategy_WholeBill_Stamp->m_StampGivingMinAmt,
			m_MemberImpl.m_Member.szInputNO,
			m_MemberImpl.m_Member.nCardNoType,strAddCol6/*m_EorderID*/,m_EMemberID,m_EMOrderStatus,
			m_MemberImpl.m_Member.szCardTypeCode,m_MemberImpl.m_Member.szEcardno,m_MemberImpl.m_Member.szUserID,
						m_MemberImpl.m_Member.szMemberCardLevel,m_MemberImpl.m_Member.szMemberCardchannel,m_strPromoterID);//把会员的整单打折也算到里面去m_fMemDiscount); //会员平台
		if ( pDB->Execute2(strSQL) == 0)//aori change 2011-3-22 16:51:36 优化clientdb ：删除execute2
			return false;

		//保存销售明细
		int nItemID = 0;
		double fSaleQuantity = 0.0f;
		bool bHasCanceledItems = false;
		double fAddDiscount = GetItemDiscount();

		for ( merchInfo = m_vecSaleMerch.begin(); merchInfo != m_vecSaleMerch.end(); ++merchInfo ) {
			//取消商品保存在操作记录Remark里, FunID = 1 --- 取消的商品
			if ( merchInfo->nMerchState == RS_CANCEL ) {
				Sleep(5);//延迟1毫秒执行，防止时间主键重复
				m_pADB->SetOperateRecord(OPT_CANCELITEM, nUserID, merchInfo->szPLU);
				merchInfo->nSID=merchInfo-m_vecSaleMerch.begin();merchInfo->SetCancelSaleItem(m_stEndSaleDT, nSaleID, nUserID,3,NULL);
				bHasCanceledItems = true;
			} else {
				nItemID++;

				double fActualPrice = merchInfo->fActualPrice;
				CString str_fSaleAmt_BeforePromotion;
				str_fSaleAmt_BeforePromotion.Format("%.2f",merchInfo->fSaleAmt_BeforePromotion);
				//计算fSaleQuantity
				if ( merchInfo->nManagementStyle != MS_DP ) //金额类商品//aori 2011-9-2 17:29:03 产品价格数量生成 优化
				{
					
					if(merchInfo->bFixedPriceAndQuantity||merchInfo->bPriceInBarcode )//称重
					{
						if (	merchInfo->fSaleQuantity >0)
						{ 
							if (merchInfo->nMerchType==5 || merchInfo->nMerchType==6)//联营金额类商品
							{
								fSaleQuantity = merchInfo->nSaleCount;
								fActualPrice = merchInfo->fActualPrice;
							}
							else
							{
								fSaleQuantity = merchInfo->fSaleQuantity;
							}
						} 
						else
						{
							if ( merchInfo->fRetailPrice  == 0.0f)
								fSaleQuantity = merchInfo->nSaleCount;
							else 
							{
								fSaleQuantity = merchInfo->fSaleAmt / merchInfo->fRetailPrice;
								fActualPrice = merchInfo->fRetailPrice;
							}
						}
					
					}else//end if ( merchInfo->nManagementStyle != MS_DP ) //金额类商品
					{ fSaleQuantity = merchInfo->fSaleQuantity/*nSaleCount*/; }	

			
				} //end if ( merchInfo->nManagementStyle != MS_DP ) //金额类商品
				else 
				{
					fSaleQuantity = merchInfo->fSaleQuantity;//(double)merchInfo->nSaleCount;
				}

				//如果是手动打折，打印出手动打折情况
				if ( merchInfo->nMerchState == RS_DS_MANUAL ) {
					if ( merchInfo->nLowLimit == SDS_RATE ) {
// 						if(merchInfo->bFixedPriceAndQuantity)
// 						{
// 							dTemp = (merchInfo->fActualPrice * merchInfo->fSaleQuantity) * (100.0f - merchInfo->fZKPrice) / 100.0f;
// 						}
// 						else
// 						{
// 							dTemp = (merchInfo->fActualPrice * merchInfo->nSaleCount) * (100.0f - merchInfo->fZKPrice) / 100.0f;
// 						}
						//数量就记在fSaleQuantity中 [dandan.wu 2016-4-26]
						dTemp = (merchInfo->fActualPrice * merchInfo->fSaleQuantity) * (100.0f - merchInfo->fZKPrice) / 100.0f;
						ReducePrecision(dTemp);
					} else {
						dTemp = merchInfo->fZKPrice;
					}
					strManualDiscount.Format("%.4f", dTemp);
				} else {
					strManualDiscount.Format("NULL");
				}
				double fManualDiscount = merchInfo->fManualDiscount;

				//如果商品属于促销但是又没有到促销的限制范围内，重新设置商品促销标志
				if(merchInfo->nMerchState == RS_NORMAL && merchInfo->nDiscountStyle != DS_NO)
				{
					merchInfo->nDiscountStyle = DS_NO;
				}
				//折扣取整到分
				double fTempSaleDiscount = merchInfo->fSaleDiscount;
				if(fabs(ReducePrecision(fTempSaleDiscount)) > 0)
				{
					fTempSaleDiscount += fAddDiscount;
					fAddDiscount = 0.0f;
				}
				if(merchInfo->bIsDiscountLabel )
					merchInfo->fBackDiscount = 	(merchInfo->fSysPrice - merchInfo->fActualPrice) * merchInfo->fSaleQuantity/*nSaleCount*/;
				else
					merchInfo->fBackDiscount = 	(merchInfo->fSysPrice - fActualPrice) * merchInfo->fSaleQuantity/*nSaleCount*/;
				
				//增加IsDiscountLabel, BackDiscount字段
				check_lock_saleitem_temp();//aori later 2011-8-7 8:38:11  proj2011-8-3-1 A 用 mutex等判断两个checkout之间 是否出现了update事件
// 				strSQL.Format("INSERT INTO SaleItem%s(SaleDT,ItemID,SaleID,"
// 					"PLU,RetailPrice,SaleCount,SaleQuantity,SaleAmt,"
// 					"PromoStyle,MakeOrgNO,PromoID, SalesHumanID, NormalPrice,ScanPLU, AddCol4,AddCol2,OrderID,ArtID,OrderItemID,CondPromDiscount,MemDiscAfterProm,ManualDiscount) VALUES ("//,StampCount//aori add 2011-8-30 10:13:21 proj2011-8-30-1 印花需求
// 					"'%s',%d,%ld,'%s',%.4f,%ld,%.4f,%.4f,%d,%d,%d,%d,%.4f,'%s','%d','%d','%s','%s','%s',%d,%d,%.4f)",//2013-2-27 15:30:11 proj2013-1-23-1 称重商品 加入 会员促销、限购,%d//aori add 2011-8-30 10:13:21 proj2011-8-30-1 印花需求
// 					szTableDTExt, strSaleDT, nItemID, nSaleID, 
// 					merchInfo->szPLU, fActualPrice, merchInfo->nSaleCount, 
// 					fSaleQuantity, merchInfo->fSaleAmt,
// 					merchInfo->nPromoStyle, 
// 					merchInfo->nMakeOrgNO, merchInfo->nPromoID,
// 					merchInfo->nSimplePromoID,merchInfo->fSysPrice,merchInfo->szPLUInput,merchInfo->scantime,merchInfo->IncludeSKU,
// 					merchInfo->strOrderNo,merchInfo->strArtID,merchInfo->strOrderItemID,merchInfo->dCondPromDiscount,merchInfo->dMemDiscAfterProm, merchInfo->fManualDiscount);//,merchInfo->StampCount//aori add 2011-8-30 10:13:21 proj2011-8-30-1 印花需求
					//若打印发票，将商品的发票信息保存到月表中[dandan.wu 2016-10-14]
				double dbInvoiceAmt = 0.0;

				if ( GetSalePos()->m_params.m_bAllowInvoicePrint )
				{
					//发票金额，若为订单商品，为0 [dandan.wu 2016-11-10]		
					if ( !merchInfo->strOrderNo.IsEmpty() )
					{
						dbInvoiceAmt = 0.0;
					}
					else
					{
						dbInvoiceAmt = merchInfo->fSaleAmt;
					}
				}
				
				double tempMemDiscAfterProm;
				if (merchInfo->dMemDiscAfterProm > DBL_EPSILON)
				{
					tempMemDiscAfterProm = merchInfo->dMemDiscAfterProm;
				} 
				else
				{
					tempMemDiscAfterProm = 0;
				}

				strSQL.Format("INSERT INTO SaleItem%s(SaleDT,ItemID,SaleID,"
					"PLU,RetailPrice,SaleCount,SaleQuantity,SaleAmt,"
					"PromoStyle,MakeOrgNO,PromoID,ManualDiscount, SalesHumanID, MemberDiscount,NormalPrice,ScanPLU,StampCount" 
					",AddCol1, AddCol4,AddCol2,ItemType,OrderID,CondPromDiscount,ArtID,OrderItemID,MemDiscAfterProm"
					",InvoiceEndNo,InvoiceCode,InvoicePageCount,InvoiceAmt"
					") VALUES ('%s',%d,%ld,'%s',%.4f,%ld,%.4f,%.4f,%d,%d,%d,%.4f,%d,%.4f,%.2f,'%s',%d" //%ld to %.4f2013-2-27 15:30:11 proj2013-1-23-1 称重商品 加入 会员促销、限购
					",'%s','%d','%d',%d,'%s',%.4f,'%s','%s',%.4f" 
					",'%s','%s',%d,%.2f"
					")",
					szTableDTExt, strSaleDT, nItemID, nSaleID, 
					merchInfo->szPLU, fActualPrice, merchInfo->nSaleCount, 
					fSaleQuantity, merchInfo->fSaleAmt,
					merchInfo->nPromoStyle, 
					merchInfo->nMakeOrgNO, merchInfo->nPromoID,
					merchInfo->fManualDiscount,merchInfo->nSimplePromoID,merchInfo->fMemberShipPoint,merchInfo->fSysPrice,merchInfo->szPLUInput,merchInfo->StampCount 
					,str_fSaleAmt_BeforePromotion ,merchInfo->scantime ,merchInfo->IncludeSKU, merchInfo->nItemType, merchInfo->strOrderNo,merchInfo->dCondPromDiscount,
					merchInfo->strArtID,merchInfo->strOrderItemID,tempMemDiscAfterProm
					,merchInfo->strInvoiceEndNo,m_strInvoiceCode,m_nInvoiceTotalPage,dbInvoiceAmt);				
				if ( pDB->Execute2(strSQL) == 0 )//aori change 2011-3-22 16:51:36 优化clientdb ：删除execute2
					return false;
			}
			//如果有操作日志，发送命令给
			if ( bHasCanceledItems ) {
				GetSalePos()->m_Task.putOperateRecord(m_stEndSaleDT, true);
			}
		}
		//清除临时数据
		strSQL.Format("DELETE FROM Sale_Temp WHERE HumanID = %d", nUserID);
		pDB->Execute2(strSQL);//aori change 2011-3-22 16:51:36 优化clientdb ：删除execute2

		check_lock_saleitem_temp();strSQL.Format("DELETE FROM SaleItem_Temp WHERE HumanID = %d", nUserID);//aori later 2011-8-7 8:38:11  proj2011-8-3-1 A 用 mutex等判断两个checkout之间 是否出现了update事件
		pDB->Execute2(strSQL);//aori change 2011-3-22 16:51:36 优化clientdb ：删除execute2
		
	} catch ( CException *pe ) {
		GetSalePos()->GetWriteLog()->WriteLogError(*pe);
		pe->Delete( );
		return false;
	}
	return true;
}

//======================================================================
// 函 数 名：GetSaleEndMerchInfo()
// 功能描述：重复商品录入,获得最后录入商品
// 输入参数：
// 输出参数：void
// 创建日期：2009-11-29
// 修改日期：
// 作  者：徐鹏
// 附加说明：
//======================================================================
SaleMerchInfo * CSaleImpl::GetSaleEndMerchInfo()
{
	SaleMerchInfo * merchInfo;
	if(m_vecSaleMerch.begin()==m_vecSaleMerch.end())
	{
		m_pEndSaleMerchInfo= NULL;
		return m_pEndSaleMerchInfo;
	}
	m_pEndSaleMerchInfo=m_vecSaleMerch.end()-1;
	return m_pEndSaleMerchInfo;
}
//获得最后录入商品
// SaleMerchInfo * CSaleImpl::GetSaleEndMerchPoint()2013-2-20 17:33:11 退出销售崩溃
// {
// 	return m_pEndSaleMerchInfo;
// }
//清空最后录入商品的记录
// void CSaleImpl::EndMerchClear()2013-2-20 17:33:11 退出销售崩溃
// {
// 	m_pEndSaleMerchInfo=NULL;
// }


//======================================================================
// 函 数 名：PrintBillWHole_cancel 
// 功能描述：打印
// 输入参数：
// 输出参数：void
// 创建日期：2009-12-6 
// 修改日期：
// 作  者: mbh
// 附加说明：
//======================================================================
void CSaleImpl::PrintBillWHole_cancel()
{
	if(!GetSalePos()->GetPrintInterface()->GetPOSPrintConfig(PBID_CANCEL_BILL).Enable)
		return;

   	CPosClientDB *pDB= CDataBaseManager::getConnection(true);
    int checktmp=0;

	if ( pDB ) {

		CString strSQL;
		CDBVariant var;
		CRecordset* pRecord = NULL;
		try {
			strSQL.Format("select count(*) from Saleitem_Temp ");

			if ( pDB->Execute(strSQL, &pRecord ) > 0 ) {
				pRecord->GetFieldValue( (short)0, var);
					 checktmp = var.m_iVal;
			}
				pDB->CloseRecordset( pRecord );
		}
		catch(CException *e) 
		{
			pDB->CloseRecordset( pRecord );
			CUniTrace::Trace(*e);
			e->Delete();
		}

	}
	CDataBaseManager::closeConnection(pDB);

	if (checktmp > 0) {
		//打印机不可用则退出
		if (!GetSalePos()->GetPrintInterface()->m_bValid) return;
		GetSalePos()->GetPrintInterface()->nSetPrintBusiness(PBID_CANCEL_BILL);
		//记录日志
		GetSalePos()->GetWriteLog()->WriteLog("开始打印整单取消小票",2);//---------------------	
		//*************thinpon.小票打印(全部)**************************
		//thinpon 打印小票小计
		//如果热敏，先打印小票头和商品行
		CPrintInterface  *printer = GetSalePos()->GetPrintInterface();
		printer->nSetPrintBusiness(PBID_CANCEL_BILL);
		
		if (GetSalePos()->m_params.m_nPrinterType==1)
		{
			//小票头
			//PrintBillHeadSmall();
			printer->nPrintLine(" ");//printer->nPrintLine(" ");//aori 2011-12-30 14:42:38 proj 2011-12-22-1 combine 2.75 with 2.71 saleimpl
			printer->nPrintLine(" ");//printer->nPrintLine(" ");//aori 2011-12-30 14:42:38 proj 2011-12-22-1 combine 2.75 with 2.71 saleimpl

			GetPrintFormatAlignString("***整单取消小票***", m_szBuff, GetSalePos()->GetParams()->m_nMaxPrintLineLen, DT_CENTER);
			printer->nPrintLine(m_szBuff);//printer->nPrintLine(m_szBuff);//aori 2011-12-30 14:42:38 proj 2011-12-22-1 combine 2.75 with 2.71 saleimpl

			TPrintHead		pPrintHead;
			char *pt = m_szBuff;

			sprintf(m_szBuff, "日期:%04d%02d%02d %02d:%02d", 
			m_stSaleDT.wYear, m_stSaleDT.wMonth, m_stSaleDT.wDay, m_stSaleDT.wHour, m_stSaleDT.wMinute);
			pPrintHead.nSaleID=m_nSaleID;			//最后一笔保存的销售流水号
			pPrintHead.stSaleDT=m_stSaleDT;	//日期/时间	
			pPrintHead.nUserID=atoi(GetSalePos()->GetCasher()->GetUserCode());
			GetSalePos()->GetPrintInterface()->nPrintHead(&pPrintHead);

			for ( vector<SaleMerchInfo>::iterator merchInfo = m_vecSaleMerch.begin(); merchInfo != m_vecSaleMerch.end(); ++merchInfo)
			{
				if (merchInfo->nMerchState != RS_CANCEL)
				{
					double fSaleQuantity=0.0f, fActualPrice=0.0f;
					merchInfo->OutPut_Quantity_Price(fSaleQuantity,fActualPrice);
					PrintBillItemSmall(merchInfo,fSaleQuantity,fActualPrice);//aori optimize 2011-10-13 14:53:19 逐行打印 replace //PrintBillItemSmall(merchInfo->szPLU,szName,fSaleQuantity,fActualPrice,merchInfo->fSaleAmt); //szName.Format("%s%s",merchInfo->nPromoID>0?"(促)":"",merchInfo->szSimpleName);//pSaleItem->nSalePromoID//aori limit 2011-2-24 10:52:36
				}
			}
		}
		GetPrintFormatAlignString("***整单取消小票***", m_szBuff, GetSalePos()->GetParams()->m_nMaxPrintLineLen, DT_CENTER);
		printer->nPrintLine(m_szBuff);//printer->nPrintLine(m_szBuff);//aori 2011-12-30 14:42:38 proj 2011-12-22-1 combine 2.75 with 2.71 saleimpl
		 //打印合计
		PrintBillTotalSmall();
		//打印票尾
		//PrintBillBootomSmall();
		GetPrintFormatAlignString("***整单取消小票***", m_szBuff, GetSalePos()->GetParams()->m_nMaxPrintLineLen, DT_CENTER);
		printer->nPrintLine(m_szBuff);//printer->nPrintLine(m_szBuff);//aori 2011-12-30 14:42:38 proj 2011-12-22-1 combine 2.75 with 2.71 saleimpl
		//for (int i=0;i<GetSalePos()->GetParams()->m_nPrintFeedNum;i++)  //mabh
		//{
		//		printer->nPrintLine(" ");//printer->nPrintLine(m_szBuff);//aori 2011-12-30 14:42:38 proj 2011-12-22-1 combine 2.75 with 2.71 saleimpl
		//}
		printer->FeedPaper_End(PBID_TRANS_RECEIPT);

		//切纸 [dandan.wu 2016-3-8]
		printer->nEndPrint_m();
		GetSalePos()->GetWriteLog()->WriteLog("切纸",POSLOG_LEVEL1);
		GetSalePos()->GetWriteLog()->WriteLog("CSaleImpl::PrintBillWHole_cancel(),切纸",POSLOG_LEVEL1);

		//delete[] szBuff;
		/*************************************************************/
	//	printer->nSetPrintBusiness(PBID_TRANS_RECEIPT);
	}
	
}

// 在POS数据库查询：
// --降价促销
// SELECT plu FROM SaleMerch WHERE MemPromoID IS NOT NULL AND PLU ='商品码'
// --条件促销
// SELECT b.ItemCode FROM Promotion a,PromotionItem b 
// WHERE a.MakeOrgNO =b.MakeOrgNO AND a.PromoID =b.PromoID AND a.CustomerRange =2 
// AND GETDATE() BETWEEN a.StartDate AND a.EndDate +1 
// AND ItemCode = '商品码''
// bool CSaleImpl::IsMemberSale(SaleMerchInfo pmerinfo){
// 	CAutoDB db(true);bool bHas = false,bHas1=false;	
// 	CString strSQL;strSQL.Format("SELECT plu FROM SaleMerch WHERE (MemPromoID IS NOT NULL and MemPromoID!=0) AND PLU ='%s'", 
// 		pmerinfo.szPLU);
// 	if ( db.Execute(strSQL) > 0 ) 
// 		return true;
// 	strSQL.Format("SELECT b.ItemCode FROM Promotion a,PromotionItem b "
// 		"WHERE a.MakeOrgNO =b.MakeOrgNO AND a.PromoID =b.PromoID AND a.CustomerRange =2 "
// 		"AND GETDATE() BETWEEN a.StartDate AND a.EndDate +1 "
// 		"AND ItemCode = '%s'", 
// 		pmerinfo.szPLU);
// 	if ( db.Execute(strSQL) > 0 ) 
// 		bHas1 = true;
// 	return bHas||bHas1;
// }

int CSaleImpl::upd_CharCode()
{//修改数据库中的数据

	CString strSaleDT = ::GetFormatDTString(m_stEndSaleDT);
	const char *szTableDTExt = GetSalePos()->GetTableDTExt(&m_stEndSaleDT);

	CString strSQL;
	CPosClientDB *pDB = CDataBaseManager::getConnection(true);
	if ( !pDB )
		return false;
	
	try {
		strSQL.Format("UPDATE sale%s set charcode = 'Y' where saleid = %d ",szTableDTExt,m_nSaleID);
		pDB->Execute2( strSQL);//aori change 2011-3-22 16:51:36 优化clientdb ：删除execute2
	
	} catch( CException * e ) {
		CUniTrace::Trace(*e);
		e->Delete( );
		CDataBaseManager::closeConnection(pDB);
		return false;
	}

	CDataBaseManager::closeConnection(pDB);
	return true;
   
}


void CSaleImpl::getcadamt(int shift_no,int styleid[9])
{
	cadamt[0]=0;
	cadamt[1]=0;
	cadamt[2]=0;

	for ( vector<Tchargeamtdaily>::const_iterator cad_tmp = vectcadshift.begin(); cad_tmp != vectcadshift.end(); ++cad_tmp)
	{
		if (shift_no == cad_tmp->shift_no)
		{
			for (int i=0; i<10; i++)
			{
				if (styleid[i] == cad_tmp->StyleID)
				{
					if (cad_tmp->type==0) 
						cadamt[0]=cadamt[0] + cad_tmp->totamt;
					if (cad_tmp->type==1) 
						cadamt[1]=cadamt[1] + cad_tmp->totamt;
					if (cad_tmp->type==2) 
						cadamt[2]=cadamt[2] + cad_tmp->totamt;
				}
			}
		}
	}
}
void CSaleImpl::PrintBillPromoStrInfo()//aori add
{
	int infolen=GetSalePos()->GetPromotionInterface()->GetPromoStrInfo()->GetLength();
	if(infolen>0)
	{//是否有促销文字信息
		//PrintBillLine(GetSalePos()->GetPromotionInterface()->GetPromoStrInfo()->GetBuffer(infolen+1));//aori		
		//m_bReprint?PrintBillLine(g_STRREPRINT):1;
	}	
}
void CSaleImpl::LimitSaleTrace(int i){
	CString tt;
	TRACE("************* check i=%d,vecsize=%d vecbegin:%d vecend:%d\n",i,m_vecSaleMerch.size(),m_vecSaleMerch.begin(),m_vecSaleMerch.end());
	for ( vector<SaleMerchInfo>::iterator merchInfo = m_vecSaleMerch.begin(); merchInfo != m_vecSaleMerch.end(); ++merchInfo )
	{
		tt.Format("			i:%d  VECID:%d blim:%d  STYLE:%d  COUNT:%d  price:%d\n",
			i,merchInfo-m_vecSaleMerch.begin(),merchInfo->bLimitSale,merchInfo->LimitStyle,merchInfo->fSaleQuantity/*nSaleCount*/,merchInfo->fActualPrice);
		CUniTrace::Trace(tt);
	}
}
// void CSaleImpl::checkXianGouV2(TRIGGER_TYPE_FOR_CHECK triggertype,int nCancelID,bool bQueryBuyingUpLimit){//aori add bQueryBuyingUpLimit 2011-4-26 9:44:43 限购问题：取消商品时总是询问是否购买超限商品
// 	double fSaleAmt=0.0;int bak_Dlginputcount_for_recover=((CPosApp *)AfxGetApp())->GetSaleDlg()->m_nInputCount;//aori add 2011-10-10 15:39:47 proj2011-9-13-1 多扫 bug:checkXianGouV2改动了m_nInputCount//遍历 saleimpl.m_vecSaleMerch ，生成新item;刷新list；重新打印；//bool bneedreprint=FALSE;//aori del 5/10/2011 9:18:37 AM proj2011-5-3-1:逐行打印完善：重输会员号、 取消商品2
// 	for ( vector<SaleMerchInfo>::iterator merchInfo = m_vecSaleMerch.begin(); merchInfo != m_vecSaleMerch.end(); merchInfo++ ) {//aori change 2011-4-11 11:36:20 bug: 换会员号后删商品后加同一商品,限购错误//LimitSaleTrace(i);//aori trace test
// 		if(merchInfo->nMerchState==RS_CANCEL)continue;//aori add 2010-12-29 9:41:25 取消商品系列错误 //bneedreprint = TRUE;//aori 2011-4-25 15:58:22 aori new change cause bug：取消商品时 重打票头
// 		SaleMerchInfo tmpmerch=(merchInfo->nSID+1)>m_vecBeforReMem.size()?*merchInfo:m_vecBeforReMem[merchInfo->nSID];//*merchInfo;//aori change 2011-5-11 9:14:43 proj2011-5-3-1:逐行打印完善：重输会员号、 取消商品 4 ：逐打被取消商品时，价格不对aori add 2011-5-3 11:36:57 proj2011-5-3-1:逐行打印完善：重输会员号、 取消商品
// 		double nInputCount=merchInfo->bFixedPriceAndQuantity?merchInfo->fSaleQuantity:merchInfo->nSaleCount;//2013-2-27 15:30:11 proj2013-1-23-1 称重商品 加入 会员促销、限购
// 		if(0==nInputCount)continue;//aori add 2011-7-12 10:10:47 proj2011-7-5-2 bug: 2.63版
// 		merchInfo->nSaleCount=0;
// 		int nowSID=merchInfo->nSID;((CPosApp *)AfxGetApp())->GetSaleDlg()->m_nCurSID=nowSID;((CPosApp *)AfxGetApp())->GetSaleDlg()->m_nInputCount=nInputCount;//aori add 2011-5-11 9:14:43 proj2011-5-3-1:逐行打印完善：重输会员号、 取消商品 5 ：bug1：取消商品bug//aori re 2011-7-12 10:10:47 proj2011-7-5-2 此处的->m_nInputCount=nInputCount赋值造成连扫出问题
// 		m_stratege_xiangou.HandleLimitSaleV3(&merchInfo, (int&)nInputCount,triggertype,nCancelID,bQueryBuyingUpLimit);
// 	
// 		merchInfo->nSaleCount += nInputCount;//aori change move to handleLimitSale
// 		ReCalculate(*merchInfo);  //liaoyu 2012-6-4 ???
// 		//ReCalculate(*merchInfo);//aori del 2012-2-10 10:18:18 proj 2.79 测试反馈：条件促销时 应付错误：未能保存、恢复商品的促销前saleAmt信息。db增加字段保存该信息
// 		fSaleAmt = merchInfo->fSaleAmt;//
// 		IsLimitSale(merchInfo);//aori test
// 		//aori add begin 2011-5-3 11:36:57 proj2011-5-3-1:逐行打印完善：重输会员号、 取消商品
// 		if(GetSalePos()->m_params.m_nPrinterType==2 
// 		&&triggertype!=TRIGGER_TYPE_InputMemberCode//aori add 2011-5-11 9:14:43 proj2011-5-3-1:逐行打印完善：重输会员号、 取消商品 3 ：输入会员逐打和限购逐打合并在一起
// 		&&(merchInfo->nSaleCount!=tmpmerch.nSaleCount|| merchInfo->fActualPrice!=tmpmerch.fActualPrice)){
// 			if (!CanChangePrice()&& CanChangeCount() && !IsDiscountLabelBar)((CPosApp *)AfxGetApp())->GetSaleDlg()->PrintSaleZhuHang(merchInfo);//aori optimize 2011-10-13 14:53:19 逐行打印
// 			PrintCancelItem(tmpmerch);
// 		}//aori add end 2011-5-3 11:36:57 proj2011-5-3-1:逐行打印完善：重输会员号、 取消商品
// 		SaleMerchInfo merchInfo_log=*merchInfo;
// 		if (merchInfo->nManagementStyle == MS_DP){merchInfo_log.nSaleCount=nInputCount;WriteSaleItemLog(merchInfo_log,0,5);
// 		}else if (merchInfo->nMerchType!=3 && merchInfo->nMerchType!=6 && !( merchInfo->bPriceInBarcode==0  ) ){//aori del && !merchInfo->bFixedPriceAndQuantityproj2013-1-23-1 称重商品 加入 会员促销、限购2013-1-23 17:02:40 
// 			if (merchInfo->bPriceInBarcode)merchInfo_log.fSaleQuantity=nInputCount;
// 			WriteSaleItemLog(merchInfo_log,0,5);
// 			}
// 			if ( !CanChangePrice()){
// 				UpdateItemInTemp(merchInfo->nSID);//aori checkxiangou后merchinfo的saleHumanID变成空了2011-9-16 10:24:26 proj 2011-9-16-1 2011-9-16 10:26:42 会员+100401。1+282701。60+103224.5 + 取消100401时 限购的103224出错
// 			}
// 		merchInfo=m_vecSaleMerch.begin()+nowSID;
// 		}
// 	
// 	CSaleDlg *pSaleDlg=((CPosApp *)AfxGetApp())->GetSaleDlg();
// 	pSaleDlg->m_nCurSID=m_vecSaleMerch.size()-1;if(pSaleDlg->m_nCurSID==-1)return;//aori add return 2011-5-13 10:32:55 bug:错误显示，上一笔条件促销销售总价信息//aori add2011-4-11 11:36:20 bug: 换会员号后删商品后加同一商品,限购错误 :
// 	pSaleDlg->m_nInputCount=bak_Dlginputcount_for_recover;//aori add 2011-10-10 15:39:47 proj2011-9-13-1 多扫 bug:checkXianGouV2改动了m_nInputCount
// 	pSaleDlg->RefreshWholeList();
// 	pSaleDlg->m_ctrlList.SelectRow(m_vecSaleMerch.size()-1);
// 	double fTotal = GetTotalAmt();
// 	pSaleDlg->m_ctrlShouldIncome.Display( fTotal );	
// 	sprintf(m_szBuff,GetSalePos()->m_params.m_bCDChinese ? "价格: %.2f" : "Price: %.2f", fSaleAmt);
// 	sprintf(m_szBuff + 64,GetSalePos()->m_params.m_bCDChinese ? "总额: %.2f" : "Total: %.2f", fTotal );
// 	pSaleDlg->m_pCD->Display( 2,fTotal );
// 	//if(	bneedreprint)ReprintAll();//aori del 5/10/2011 9:18:37 AM proj2011-5-3-1:逐行打印完善：重输会员号、 取消商品2
// }
// void CSaleImpl::checkXianGou_bangcheng(TRIGGER_TYPE_FOR_CHECK triggertype,int nCancelID,bool bQueryBuyingUpLimit){//aori add bQueryBuyingUpLimit 2011-4-26 9:44:43 限购问题：取消商品时总是询问是否购买超限商品
// 	double fSaleAmt=0.0;int bak_Dlginputcount_for_recover=((CPosApp *)AfxGetApp())->GetSaleDlg()->m_nInputCount;//aori add 2011-10-10 15:39:47 proj2011-9-13-1 多扫 bug:checkXianGouV2改动了m_nInputCount//遍历 saleimpl.m_vecSaleMerch ，生成新item;刷新list；重新打印；//bool bneedreprint=FALSE;//aori del 5/10/2011 9:18:37 AM proj2011-5-3-1:逐行打印完善：重输会员号、 取消商品2
// 	for ( vector<SaleMerchInfo>::iterator merchInfo = m_vecSaleMerch.begin(); merchInfo != m_vecSaleMerch.end(); merchInfo++ ) {//aori change 2011-4-11 11:36:20 bug: 换会员号后删商品后加同一商品,限购错误//LimitSaleTrace(i);//aori trace test
// 		if(merchInfo->nMerchState==RS_CANCEL)continue;//aori add 2010-12-29 9:41:25 取消商品系列错误 //bneedreprint = TRUE;//aori 2011-4-25 15:58:22 aori new change cause bug：取消商品时 重打票头
// 		SaleMerchInfo tmpmerch=(merchInfo->nSID+1)>m_vecBeforReMem.size()?*merchInfo:m_vecBeforReMem[merchInfo->nSID];//*merchInfo;//aori change 2011-5-11 9:14:43 proj2011-5-3-1:逐行打印完善：重输会员号、 取消商品 4 ：逐打被取消商品时，价格不对aori add 2011-5-3 11:36:57 proj2011-5-3-1:逐行打印完善：重输会员号、 取消商品
// 		int nowSID=merchInfo->nSID;
// 		((CPosApp *)AfxGetApp())->GetSaleDlg()->m_nCurSID=nowSID;
// 		double nInputCount;
// 		if(	merchInfo->bFixedPriceAndQuantity){//磅秤商品限购
// 			nInputCount=merchInfo->fSaleQuantity;//2013-2-27 15:30:11 proj2013-1-23-1 称重商品 加入 会员促销、限购
// 			if(0==nInputCount)continue;//aori add 2011-7-12 10:10:47 proj2011-7-5-2 bug: 2.63版
// 			merchInfo->fSaleQuantity=0;
// 			((CPosApp *)AfxGetApp())->GetSaleDlg()->m_nInputCount=1;//aori add 2011-5-11 9:14:43 proj2011-5-3-1:逐行打印完善：重输会员号、 取消商品 5 ：bug1：取消商品bug//aori re 2011-7-12 10:10:47 proj2011-7-5-2 此处的->m_nInputCount=nInputCount赋值造成连扫出问题
// 			HandleLimitSale_bangcheng(&merchInfo, nInputCount,triggertype,nCancelID,bQueryBuyingUpLimit);
// 			merchInfo->fSaleQuantity += nInputCount;//aori change move to handleLimitSale
// 
// 		}else{		//普通商品限购
// 			nInputCount=merchInfo->nSaleCount;//2013-2-27 15:30:11 proj2013-1-23-1 称重商品 加入 会员促销、限购
// 			if(0==nInputCount)continue;//aori add 2011-7-12 10:10:47 proj2011-7-5-2 bug: 2.63版
// 			merchInfo->nSaleCount=0;
// 			((CPosApp *)AfxGetApp())->GetSaleDlg()->m_nInputCount=nInputCount;//aori add 2011-5-11 9:14:43 proj2011-5-3-1:逐行打印完善：重输会员号、 取消商品 5 ：bug1：取消商品bug//aori re 2011-7-12 10:10:47 proj2011-7-5-2 此处的->m_nInputCount=nInputCount赋值造成连扫出问题
// 			HandleLimitSaleV2(&merchInfo, (int&)nInputCount,triggertype,nCancelID,bQueryBuyingUpLimit);
// 			merchInfo->nSaleCount += nInputCount;//aori change move to handleLimitSale
// 		}
// 		ReCalculate(*merchInfo);  //liaoyu 2012-6-4 ???
// 		fSaleAmt = merchInfo->fSaleAmt;//
// 		IsLimitSale(merchInfo);//aori test
// 		//aori add begin 2011-5-3 11:36:57 proj2011-5-3-1:逐行打印完善：重输会员号、 取消商品
// 		if(GetSalePos()->m_params.m_nPrinterType==2 
// 		&&triggertype!=TRIGGER_TYPE_InputMemberCode//aori add 2011-5-11 9:14:43 proj2011-5-3-1:逐行打印完善：重输会员号、 取消商品 3 ：输入会员逐打和限购逐打合并在一起
// 		&&((merchInfo->bFixedPriceAndQuantity?merchInfo->fSaleQuantity!=tmpmerch.fSaleQuantity:merchInfo->nSaleCount!=tmpmerch.nSaleCount)
// 			|| merchInfo->fActualPrice!=tmpmerch.fActualPrice)){
// 			if (!CanChangePrice()&& CanChangeCount() && !IsDiscountLabelBar)((CPosApp *)AfxGetApp())->GetSaleDlg()->PrintSaleZhuHang(merchInfo);//aori optimize 2011-10-13 14:53:19 逐行打印
// 			PrintCancelItem(tmpmerch);
// 		}//aori add end 2011-5-3 11:36:57 proj2011-5-3-1:逐行打印完善：重输会员号、 取消商品
// 		SaleMerchInfo merchInfo_log=*merchInfo;
// 		if (merchInfo->nManagementStyle == MS_DP){merchInfo_log.nSaleCount=nInputCount;WriteSaleItemLog(merchInfo_log,0,5);
// 		}else if (merchInfo->nMerchType!=3 && merchInfo->nMerchType!=6 && !( merchInfo->bPriceInBarcode==0  ) ){//aori del && !merchInfo->bFixedPriceAndQuantityproj2013-1-23-1 称重商品 加入 会员促销、限购2013-1-23 17:02:40 
// 			if (merchInfo->bPriceInBarcode)merchInfo_log.fSaleQuantity=nInputCount;
// 			WriteSaleItemLog(merchInfo_log,0,5);
// 		}
// 		if ( !CanChangePrice())UpdateItemInTemp(merchInfo->nSID);
// 		merchInfo=m_vecSaleMerch.begin()+nowSID;
// 	}	
// 	CSaleDlg *pSaleDlg=((CPosApp *)AfxGetApp())->GetSaleDlg();
// 	pSaleDlg->m_nCurSID=m_vecSaleMerch.size()-1;if(pSaleDlg->m_nCurSID==-1)return;//aori add return 2011-5-13 10:32:55 bug:错误显示，上一笔条件促销销售总价信息//aori add2011-4-11 11:36:20 bug: 换会员号后删商品后加同一商品,限购错误 :
// 	pSaleDlg->m_nInputCount=bak_Dlginputcount_for_recover;//aori add 2011-10-10 15:39:47 proj2011-9-13-1 多扫 bug:checkXianGouV2改动了m_nInputCount
// 	pSaleDlg->RefreshWholeList();
// 	pSaleDlg->m_ctrlList.SelectRow(m_vecSaleMerch.size()-1);
// 	double fTotal = GetTotalAmt();
// 	pSaleDlg->m_ctrlShouldIncome.Display( fTotal );	
// 	sprintf(m_szBuff,GetSalePos()->m_params.m_bCDChinese ? "价格: %.2f" : "Price: %.2f", fSaleAmt);
// 	sprintf(m_szBuff + 64,GetSalePos()->m_params.m_bCDChinese ? "总额: %.2f" : "Total: %.2f", fTotal );
// 	pSaleDlg->m_pCD->Display( 2,fTotal );
// 	//if(	bneedreprint)ReprintAll();//aori del 5/10/2011 9:18:37 AM proj2011-5-3-1:逐行打印完善：重输会员号、 取消商品2
// }
// void CSaleImpl::SetBeMember(bool beMember){//,TMember memberInfo){//aori add
// 	m_bMember = beMember;
// 	//checkXianGouV2(TRIGGER_TYPE_InputMemberCode);//
// 	ReprintAll();//aori add 2011-5-11 9:14:43 proj2011-5-3-1:逐行打印完善：重输会员号、 取消商品 3 ：输入会员逐打和限购逐打合并在一起
// }

bool CSaleImpl::IsMember()
{//aori add
	return strlen(GetMember().szMemberCode);
}

// bool CSaleImpl::FindLimitInfo(SaleMerchInfo* salemerchInfo){//aori add
// 	//如是正常商品则直接返回
// 	//先在本地找limit信息
// 	bool bLocalExist=false;
// 	m_pLimitInfo = NULL;//aori add  2010-12-29 9:41:25 取消商品系列错误
// 	for(std::vector<LimitSaleInfo>::iterator iterLimInfo = m_VectLimitInfo.begin();iterLimInfo!=m_VectLimitInfo.end();iterLimInfo++){
// 		if(strcmp(iterLimInfo->szPLU,salemerchInfo->szPLU)==0
// 			&& iterLimInfo->bMember==IsMember() // 2010-12-29 8:49:34 区分为会员限购、非会员限购
// 			){//
// 			m_pLimitInfo=iterLimInfo;
// 			bLocalExist=true;
// 		}
// 	}
// 	if(!bLocalExist){
// 		//在本地找不到limit信息，就连server端得到信息，并保存到本地。
// 		int LimitAlreadySaleCount = 0;
// 		LimitSaleInfo limInfo;
// 		int rtn=	GetSalePos()->m_downloader.GetLimitAlreadySaleCount( salemerchInfo ,m_MemberImpl.m_Member.szMemberCode,LimitAlreadySaleCount);
// 		if (SOCKET_ERROR==rtn){
// 			CString msg;
// 			msg.Format("获取历史限购数时通讯失败,假设历史数为0");
// 			//AfxMessageBox(msg);
// 		}
// 		//strcpy(limInfo.szPLU,salemerchInfo->szPLU);
// 		memcpy(limInfo.szPLU,salemerchInfo->szPLU,24);
// 		limInfo.bMember = IsMember();
// 		//limInfo.LimitStyle = salemerchInfo->LimitStyle;//aori test del 2010-12-29 16:55:47 test 取消 限购信息中的limitsaleinfo：：limitstyle
// 		//limInfo.LimitQty = salemerchInfo->LimitQty;
// 		limInfo.LimitLEFTcount=(LimitAlreadySaleCount < salemerchInfo->LimitQty?salemerchInfo->LimitQty - LimitAlreadySaleCount:0);
// 		m_VectLimitInfo.push_back(limInfo);
// 		m_pLimitInfo=&m_VectLimitInfo[m_VectLimitInfo.size()-1];
// 	}	
// 	return bLocalExist;
// }
// void CSaleImpl::HandleLimitSaleV2(vector<SaleMerchInfo>::iterator * ppsalemerchInfo,int& nInputCount,//2013-2-27 15:30:11 proj2013-1-23-1 称重商品 加入 会员促销、限购
// 								 TRIGGER_TYPE_FOR_CHECK TriggerForCheck,int nCancelID,bool bQueryBuyingUpLimit){//aori add bcheck 是否是全扫描（member变化||取消限购商品时出发全扫）；bcancellimitmerch是否是取消限购商品//aori add bQueryBuyingUpLimit 2011-4-26 9:44:43 限购问题：取消商品时总是询问是否购买超限商品
// 	vector<SaleMerchInfo>::iterator salemerchInfo=*ppsalemerchInfo;
// 	if(!((IsMember() 
// 			&& (salemerchInfo->LimitStyle>LIMITSTYLE_NoLimit)//&&salemerchInfo->LimitStyle<=LIMITSTYLE_UPLIMIT)//aori add 2010-12-31 17:13:33 需增加限购类型：5：会员超限 6：非会员超限 7：会员全体超限
// 					&& TRIGGER_TYPE_NoCheck==TriggerForCheck)
// 		||(!IsMember() 
// 			&&(LIMITSTYLE_ALLXIAOPIAO<=salemerchInfo->LimitStyle) //aori add 2010-12-31 17:13:33 需增加限购类型：5：会员超限 6：非会员超限 7：会员全体超限
// 					&& TRIGGER_TYPE_NoCheck==TriggerForCheck)
// 		||(salemerchInfo->LimitStyle>LIMITSTYLE_NoLimit&&salemerchInfo->LimitStyle<=LIMITSTYLE_ALLXIAOPIAO
// 				&& CHECKEDHISTORY_NoCheck==salemerchInfo->CheckedHistory 
// 					&& TRIGGER_TYPE_InputMemberCode==TriggerForCheck)
// 		||( (LIMITSTYLE_ALLXIAOPIAO<=salemerchInfo->LimitStyle)//aori add 增加对超限商品的handle 因为：2010-12-31 17:13:33 need 当limitstyle==4（全体小票限购）&& bmember && memprice==retailprice 时 超限商品 actualprice应取Normalprice
// 				&& CHECKEDHISTORY_NoMemLimit==salemerchInfo->CheckedHistory 
// 					&& TRIGGER_TYPE_InputMemberCode==TriggerForCheck)
// 		||( LIMITSTYLE_UPLIMIT<=salemerchInfo->LimitStyle//aori add 2010-12-31 17:13:33 需增加限购类型：5：会员超限 6：非会员超限 7：会员全体超限
// 					&& TRIGGER_TYPE_CancelLimitSale==TriggerForCheck)
// 	)||salemerchInfo->bIsDiscountLabel ){//aori delproj2013-1-23-1 称重商品 加入 会员促销、限购2013-1-23 17:02:40  ||salemerchInfo->bFixedPriceAndQuantity//aori add 2010-12-30 14:30:39 生鲜不应该参加限购
// 		return;
// 	}
// 	IsLimitSale( salemerchInfo );//aori add 2010-12-29 9:41:25
// 	if(TRIGGER_TYPE_InputMemberCode==TriggerForCheck){
// 		salemerchInfo->CheckedHistory= CHECKEDHISTORY_MemberLimit;
// 	}else if(TRIGGER_TYPE_NoCheck==TriggerForCheck){
// 		salemerchInfo->CheckedHistory=IsMember()?CHECKEDHISTORY_MemberLimit:CHECKEDHISTORY_NoMemLimit;
// 	}else if(TRIGGER_TYPE_CancelLimitSale==TriggerForCheck){
// 		if(salemerchInfo->nMerchID ==  m_vecSaleMerch[nCancelID].nMerchID)
// 			salemerchInfo->LimitStyle = m_vecSaleMerch[nCancelID].LimitStyle;
// 	}
// 	FindLimitInfo( salemerchInfo);
// 	//if(salemerchInfo->nSaleCount+nSaleCount<=m_pLimitInfo->LimitLEFTcount){//新增的销量在可限购范围内,不需新增正常merchinfo直接返回,更新本地limitinfo
// 	if(nInputCount<=m_pLimitInfo->LimitLEFTcount){//新增的销量在可限购范围内,不需新增正常merchinfo直接返回,更新本地limitinfo
// 		m_pLimitInfo->LimitLEFTcount -=nInputCount;
// 		return;
// 
// 	}else{//需增新的正常merchinfo，并处理限购merchinfo。然后让salemerchinfo指向新的merchinfo，更改nSaleCount值
// 		//if(TRIGGER_TYPE_CancelLimitSale!=TriggerForCheck){//aori del 取消商品机制有问题 暂时封闭 用全扫描代替
// 		if(bQueryBuyingUpLimit){//aori add bQueryBuyingUpLimit 2011-4-26 9:44:43 限购问题：取消商品时总是询问是否购买超限商品
// 			CConfirmLimitSaleDlg confirmDlg;
// 			confirmDlg.m_strConfirm.Format("已超限，是否购买超限部分商品");
// 			if ( confirmDlg.DoModal() == IDCANCEL ) {
// 				nInputCount=m_pLimitInfo->LimitLEFTcount;
// 				m_pLimitInfo->LimitLEFTcount -=nInputCount;
// 				return;
// 			}//aori add end 2010-12-25 13:51:48 later 询问是否购买超限商品。
// 		}
// 		double oldandnewTOTALCOUNT=nInputCount;
// 		CSaleDlg *pSaleDlg=((CPosApp *)AfxGetApp())->GetSaleDlg();
// 		(salemerchInfo->nSaleCount)+=(m_pLimitInfo->LimitLEFTcount);//bug???aori later LimitLEFTcount为零时???
// 		if(salemerchInfo->nSaleCount){//
// 			ReCalculate(*salemerchInfo);
// 			IsLimitSale(salemerchInfo);//aori test
// 			SaleMerchInfo merchInfo_log=*salemerchInfo;
// 			if (salemerchInfo->nManagementStyle == MS_DP){
// 				merchInfo_log.nSaleCount=m_pLimitInfo->LimitLEFTcount;WriteSaleItemLog(merchInfo_log,0,6);
// 				}else if (salemerchInfo->bPriceInBarcode&&salemerchInfo->nMerchType!=3 && salemerchInfo->nMerchType!=6 && !( salemerchInfo->bPriceInBarcode==0  ) ){//proj2013-1-23-1 称重商品 加入 会员促销、限购2013-1-23 17:02:40 aori del && !salemerchInfo->bFixedPriceAndQuantity
// 				merchInfo_log.fSaleQuantity=m_pLimitInfo->LimitLEFTcount;WriteSaleItemLog(merchInfo_log,0,6);
// 			}		
// 			nInputCount = salemerchInfo->nSaleCount;//限购型商品的数量将要被打印出来，原来的总数是限购和超出限购的总和。这个nSaleCount就是m_nsaleCount的引用。
// 			pSaleDlg->RefreshList(salemerchInfo->nSID);//pSaleDlg->m_nCurSID);	
// 			pSaleDlg->m_nInputCount=m_pLimitInfo->LimitLEFTcount;//aori add 2011-4-26 15:56:03 逐行打印新购商品bug
// 				if ( !CanChangePrice()){
// 				UpdateItemInTemp(salemerchInfo->nSID);//pSaleDlg->m_nCurSID);
// 				if (GetSalePos()->m_params.m_nPrinterType==2
// 				&&TriggerForCheck!=TRIGGER_TYPE_InputMemberCode)//aori add 2011-5-11 9:14:43 proj2011-5-3-1:逐行打印完善：重输会员号、 取消商品 3 ：输入会员逐打和限购逐打合并在一起
// 					pSaleDlg->PrintSaleZhuHang(salemerchInfo);//aori optimize 2011-10-13 14:53:19 逐行打印//aori change 5/10/2011 9:18:37 AM proj2011-5-3-1:逐行打印完善：重输会员号、 取消商品2
// 			}
// 			pSaleDlg->m_nInputCount=oldandnewTOTALCOUNT-m_pLimitInfo->LimitLEFTcount;//aori del 2011-4-26 15:37:07 del test m_nSameMerchContinueInputCount
// 		}
// 		//增加正常merchinfo即超出限购数后的merchinfo
// 		pSaleDlg->m_nCurSID = AddUPLimitsalemerch(&salemerchInfo,salemerchInfo->nSaleCount==0?FALSE:TRUE);//函数功能:增加新的正常商品merchinfo。返回后salemerchInfo指向新的正常商品merchinfo，返回值赋给m_nCurSID，然后更新nsalecount
// 		nInputCount=oldandnewTOTALCOUNT-m_pLimitInfo->LimitLEFTcount;//更新saledlg的nSaleCount 此nSaleCount是新正常商品的，用于在modify正常商品的
// 		*ppsalemerchInfo=salemerchInfo;
// 		m_pLimitInfo->LimitLEFTcount=0;
// 	}
// }
// void CSaleImpl::HandleLimitSaleV3(vector<SaleMerchInfo>::iterator * ppsalemerchInfo,int& nInputCount,//2013-2-27 15:30:11 proj2013-1-23-1 称重商品 加入 会员促销、限购
// 								 TRIGGER_TYPE_FOR_CHECK TriggerForCheck,int nCancelID,bool bQueryBuyingUpLimit){//aori add bcheck 是否是全扫描（member变化||取消限购商品时出发全扫）；bcancellimitmerch是否是取消限购商品//aori add bQueryBuyingUpLimit 2011-4-26 9:44:43 限购问题：取消商品时总是询问是否购买超限商品
// 	vector<SaleMerchInfo>::iterator salemerchInfo=*ppsalemerchInfo;
// 	if(!((IsMember() 
// 			&& (salemerchInfo->LimitStyle>LIMITSTYLE_NoLimit)//&&salemerchInfo->LimitStyle<=LIMITSTYLE_UPLIMIT)//aori add 2010-12-31 17:13:33 需增加限购类型：5：会员超限 6：非会员超限 7：会员全体超限
// 					&& TRIGGER_TYPE_NoCheck==TriggerForCheck)
// 		||(!IsMember() 
// 			&&(LIMITSTYLE_ALLXIAOPIAO<=salemerchInfo->LimitStyle) //aori add 2010-12-31 17:13:33 需增加限购类型：5：会员超限 6：非会员超限 7：会员全体超限
// 					&& TRIGGER_TYPE_NoCheck==TriggerForCheck)
// 		||(salemerchInfo->LimitStyle>LIMITSTYLE_NoLimit&&salemerchInfo->LimitStyle<=LIMITSTYLE_ALLXIAOPIAO
// 				&& CHECKEDHISTORY_NoCheck==salemerchInfo->CheckedHistory 
// 					&& TRIGGER_TYPE_InputMemberCode==TriggerForCheck)
// 		||( (LIMITSTYLE_ALLXIAOPIAO<=salemerchInfo->LimitStyle)//aori add 增加对超限商品的handle 因为：2010-12-31 17:13:33 need 当limitstyle==4（全体小票限购）&& bmember && memprice==retailprice 时 超限商品 actualprice应取Normalprice
// 				&& CHECKEDHISTORY_NoMemLimit==salemerchInfo->CheckedHistory 
// 					&& TRIGGER_TYPE_InputMemberCode==TriggerForCheck)
// 		||( LIMITSTYLE_UPLIMIT<=salemerchInfo->LimitStyle//aori add 2010-12-31 17:13:33 需增加限购类型：5：会员超限 6：非会员超限 7：会员全体超限
// 					&& TRIGGER_TYPE_CancelLimitSale==TriggerForCheck)
// 	)||salemerchInfo->bIsDiscountLabel ){//aori delproj2013-1-23-1 称重商品 加入 会员促销、限购2013-1-23 17:02:40  ||salemerchInfo->bFixedPriceAndQuantity//aori add 2010-12-30 14:30:39 生鲜不应该参加限购
// 		return;
// 	}
// 	IsLimitSale( salemerchInfo );//aori add 2010-12-29 9:41:25
// 	if(TRIGGER_TYPE_InputMemberCode==TriggerForCheck){
// 		salemerchInfo->CheckedHistory= CHECKEDHISTORY_MemberLimit;
// 	}else if(TRIGGER_TYPE_NoCheck==TriggerForCheck){
// 		salemerchInfo->CheckedHistory=IsMember()?CHECKEDHISTORY_MemberLimit:CHECKEDHISTORY_NoMemLimit;
// 	}else if(TRIGGER_TYPE_CancelLimitSale==TriggerForCheck){
// 		if(salemerchInfo->nMerchID ==  m_vecSaleMerch[nCancelID].nMerchID)
// 			salemerchInfo->LimitStyle = m_vecSaleMerch[nCancelID].LimitStyle;
// 	}
// 	FindLimitInfo( salemerchInfo);
// 	//if(salemerchInfo->nSaleCount+nSaleCount<=m_pLimitInfo->LimitLEFTcount){//新增的销量在可限购范围内,不需新增正常merchinfo直接返回,更新本地limitinfo
// 	if(nInputCount<=m_pLimitInfo->LimitLEFTcount){//新增的销量在可限购范围内,不需新增正常merchinfo直接返回,更新本地limitinfo
// 		m_pLimitInfo->LimitLEFTcount -=nInputCount;
// 		return;
// 
// 	}else{
// 		if(bQueryBuyingUpLimit){//aori add bQueryBuyingUpLimit 2011-4-26 9:44:43 限购问题：取消商品时总是询问是否购买超限商品
// 			CConfirmLimitSaleDlg confirmDlg;confirmDlg.m_strConfirm.Format("已超限，是否购买超限部分商品");
// 			if ( confirmDlg.DoModal() == IDCANCEL ) {
// 				nInputCount=m_pLimitInfo->LimitLEFTcount;
// 				m_pLimitInfo->LimitLEFTcount =0;
// 				return;
// 			}//aori add end 2010-12-25 13:51:48 later 询问是否购买超限商品。
// 		}
// 		double uplimitcount=nInputCount-m_pLimitInfo->LimitLEFTcount;
// 		m_pLimitInfo->LimitLEFTcount=0;
// 
// 
// 
// 
// 		
// 		double oldandnewTOTALCOUNT=nInputCount;
// 		CSaleDlg *pSaleDlg=((CPosApp *)AfxGetApp())->GetSaleDlg();
// 		(salemerchInfo->nSaleCount)+=(m_pLimitInfo->LimitLEFTcount);//bug???aori later LimitLEFTcount为零时???
// 		if(salemerchInfo->nSaleCount){//
// 			ReCalculate(*salemerchInfo);
// 			IsLimitSale(salemerchInfo);//aori test
// 			SaleMerchInfo merchInfo_log=*salemerchInfo;
// 			if (salemerchInfo->nManagementStyle == MS_DP){
// 				merchInfo_log.nSaleCount=m_pLimitInfo->LimitLEFTcount;WriteSaleItemLog(merchInfo_log,0,6);
// 				}else if (salemerchInfo->bPriceInBarcode&&salemerchInfo->nMerchType!=3 && salemerchInfo->nMerchType!=6 && !( salemerchInfo->bPriceInBarcode==0  ) ){//proj2013-1-23-1 称重商品 加入 会员促销、限购2013-1-23 17:02:40 aori del && !salemerchInfo->bFixedPriceAndQuantity
// 				merchInfo_log.fSaleQuantity=m_pLimitInfo->LimitLEFTcount;WriteSaleItemLog(merchInfo_log,0,6);
// 			}		
// 			nInputCount = salemerchInfo->nSaleCount;//限购型商品的数量将要被打印出来，原来的总数是限购和超出限购的总和。这个nSaleCount就是m_nsaleCount的引用。
// 			pSaleDlg->RefreshList(salemerchInfo->nSID);//pSaleDlg->m_nCurSID);	
// 			pSaleDlg->m_nInputCount=m_pLimitInfo->LimitLEFTcount;//aori add 2011-4-26 15:56:03 逐行打印新购商品bug
// 				if ( !CanChangePrice()){
// 				UpdateItemInTemp(salemerchInfo->nSID);//pSaleDlg->m_nCurSID);
// 				if (GetSalePos()->m_params.m_nPrinterType==2
// 				&&TriggerForCheck!=TRIGGER_TYPE_InputMemberCode)//aori add 2011-5-11 9:14:43 proj2011-5-3-1:逐行打印完善：重输会员号、 取消商品 3 ：输入会员逐打和限购逐打合并在一起
// 					pSaleDlg->PrintSaleZhuHang(salemerchInfo);//aori optimize 2011-10-13 14:53:19 逐行打印//aori change 5/10/2011 9:18:37 AM proj2011-5-3-1:逐行打印完善：重输会员号、 取消商品2
// 			}
// 			pSaleDlg->m_nInputCount=oldandnewTOTALCOUNT-m_pLimitInfo->LimitLEFTcount;//aori del 2011-4-26 15:37:07 del test m_nSameMerchContinueInputCount
// 		}
// 		//增加正常merchinfo即超出限购数后的merchinfo
// 		pSaleDlg->m_nCurSID = AddUPLimitsalemerch(&salemerchInfo,salemerchInfo->nSaleCount==0?FALSE:TRUE);//函数功能:增加新的正常商品merchinfo。返回后salemerchInfo指向新的正常商品merchinfo，返回值赋给m_nCurSID，然后更新nsalecount
// 		nInputCount=oldandnewTOTALCOUNT-m_pLimitInfo->LimitLEFTcount;//更新saledlg的nSaleCount 此nSaleCount是新正常商品的，用于在modify正常商品的
// 		*ppsalemerchInfo=salemerchInfo;
// 		m_pLimitInfo->LimitLEFTcount=0;
// 	}
// }
// 
// void CSaleImpl::HandleLimitSale_bangcheng(vector<SaleMerchInfo>::iterator * ppsalemerchInfo,double& YYquantity,//2013-2-27 15:30:11 proj2013-1-23-1 称重商品 加入 会员促销、限购
// 								 TRIGGER_TYPE_FOR_CHECK TriggerForCheck,int nCancelID,bool bQueryBuyingUpLimit){//aori add bcheck 是否是全扫描（member变化||取消限购商品时出发全扫）；bcancellimitmerch是否是取消限购商品//aori add bQueryBuyingUpLimit 2011-4-26 9:44:43 限购问题：取消商品时总是询问是否购买超限商品
// 	vector<SaleMerchInfo>::iterator salemerchInfo=*ppsalemerchInfo;
// 	if(!((IsMember() 
// 			&& (salemerchInfo->LimitStyle>LIMITSTYLE_NoLimit)//&&salemerchInfo->LimitStyle<=LIMITSTYLE_UPLIMIT)//aori add 2010-12-31 17:13:33 需增加限购类型：5：会员超限 6：非会员超限 7：会员全体超限
// 					&& TRIGGER_TYPE_NoCheck==TriggerForCheck)
// 		||(!IsMember() 
// 			&&(LIMITSTYLE_ALLXIAOPIAO<=salemerchInfo->LimitStyle) //aori add 2010-12-31 17:13:33 需增加限购类型：5：会员超限 6：非会员超限 7：会员全体超限
// 					&& TRIGGER_TYPE_NoCheck==TriggerForCheck)
// 		||(salemerchInfo->LimitStyle>LIMITSTYLE_NoLimit&&salemerchInfo->LimitStyle<=LIMITSTYLE_ALLXIAOPIAO
// 				&& CHECKEDHISTORY_NoCheck==salemerchInfo->CheckedHistory 
// 					&& TRIGGER_TYPE_InputMemberCode==TriggerForCheck)
// 		||( (LIMITSTYLE_ALLXIAOPIAO<=salemerchInfo->LimitStyle)//aori add 增加对超限商品的handle 因为：2010-12-31 17:13:33 need 当limitstyle==4（全体小票限购）&& bmember && memprice==retailprice 时 超限商品 actualprice应取Normalprice
// 				&& CHECKEDHISTORY_NoMemLimit==salemerchInfo->CheckedHistory 
// 					&& TRIGGER_TYPE_InputMemberCode==TriggerForCheck)
// 		||( LIMITSTYLE_UPLIMIT<=salemerchInfo->LimitStyle//aori add 2010-12-31 17:13:33 需增加限购类型：5：会员超限 6：非会员超限 7：会员全体超限
// 					&& TRIGGER_TYPE_CancelLimitSale==TriggerForCheck)
// 	)||salemerchInfo->bIsDiscountLabel ){//aori delproj2013-1-23-1 称重商品 加入 会员促销、限购2013-1-23 17:02:40  ||salemerchInfo->bFixedPriceAndQuantity//aori add 2010-12-30 14:30:39 生鲜不应该参加限购
// 		return;
// 	}
// 	IsLimitSale( salemerchInfo );//aori add 2010-12-29 9:41:25
// 	if(TRIGGER_TYPE_InputMemberCode==TriggerForCheck){
// 		salemerchInfo->CheckedHistory= CHECKEDHISTORY_MemberLimit;
// 	}else if(TRIGGER_TYPE_NoCheck==TriggerForCheck){
// 		salemerchInfo->CheckedHistory=IsMember()?CHECKEDHISTORY_MemberLimit:CHECKEDHISTORY_NoMemLimit;
// 	}else if(TRIGGER_TYPE_CancelLimitSale==TriggerForCheck){
// 		if(salemerchInfo->nMerchID ==  m_vecSaleMerch[nCancelID].nMerchID)
// 			salemerchInfo->LimitStyle = m_vecSaleMerch[nCancelID].LimitStyle;
// 	}
// 	FindLimitInfo( salemerchInfo);
// 	if(YYquantity<=m_pLimitInfo->LimitLEFTcount){//新增的销量在可限购范围内,不需新增正常merchinfo直接返回,更新本地limitinfo
// 		m_pLimitInfo->LimitLEFTcount -=YYquantity;
// 		return;
// 
// 	}else{//需增新的正常merchinfo，并处理限购merchinfo。然后让salemerchinfo指向新的merchinfo，更改nSaleCount值
// 		//if(TRIGGER_TYPE_CancelLimitSale!=TriggerForCheck){//aori del 取消商品机制有问题 暂时封闭 用全扫描代替
// 		if(bQueryBuyingUpLimit){//aori add bQueryBuyingUpLimit 2011-4-26 9:44:43 限购问题：取消商品时总是询问是否购买超限商品
// 			CConfirmLimitSaleDlg confirmDlg;confirmDlg.m_strConfirm.Format("已超限，是:超限部分按正常价计价，否:放弃该商品");if ( confirmDlg.DoModal() == IDCANCEL ) {
// 				YYquantity=m_pLimitInfo->LimitLEFTcount;
// 				m_pLimitInfo->LimitLEFTcount -=YYquantity;
// 				return;
// 			}//aori add end 2010-12-25 13:51:48 later 询问是否购买超限商品。
// 		}
// 		double oldandnewTOTALquantity=YYquantity;
// 		CSaleDlg *pSaleDlg=((CPosApp *)AfxGetApp())->GetSaleDlg();
// 		(salemerchInfo->fSaleQuantity)=(m_pLimitInfo->LimitLEFTcount);//bug???aori later LimitLEFTcount为零时???
// 		if(salemerchInfo->fSaleQuantity){//
// 			ReCalculate(*salemerchInfo);
// 			IsLimitSale(salemerchInfo);//aori test
// 			SaleMerchInfo merchInfo_log=*salemerchInfo;
// 			if (salemerchInfo->nManagementStyle == MS_DP){
// 				merchInfo_log.fSaleQuantity=m_pLimitInfo->LimitLEFTcount;WriteSaleItemLog(merchInfo_log,0,6);
// 				}else if (salemerchInfo->bPriceInBarcode&&salemerchInfo->nMerchType!=3 && salemerchInfo->nMerchType!=6 && !( salemerchInfo->bPriceInBarcode==0  ) ){//proj2013-1-23-1 称重商品 加入 会员促销、限购2013-1-23 17:02:40 aori del && !salemerchInfo->bFixedPriceAndQuantity
// 				merchInfo_log.fSaleQuantity=m_pLimitInfo->LimitLEFTcount;WriteSaleItemLog(merchInfo_log,0,6);
// 			}		
// 			YYquantity = salemerchInfo->fSaleQuantity;//限购型商品的数量将要被打印出来，原来的总数是限购和超出限购的总和。这个nSaleCount就是m_nsaleCount的引用。
// 			pSaleDlg->RefreshList(salemerchInfo->nSID);//pSaleDlg->m_nCurSID);	
// 			//pSaleDlg->m_nInputCount=m_pLimitInfo->LimitLEFTcount;//aori add 2011-4-26 15:56:03 逐行打印新购商品bug
// 			if ( !CanChangePrice()){
// 				UpdateItemInTemp(salemerchInfo->nSID);//pSaleDlg->m_nCurSID);
// 				if (GetSalePos()->m_params.m_nPrinterType==2
// 				&&TriggerForCheck!=TRIGGER_TYPE_InputMemberCode)//aori add 2011-5-11 9:14:43 proj2011-5-3-1:逐行打印完善：重输会员号、 取消商品 3 ：输入会员逐打和限购逐打合并在一起
// 					pSaleDlg->PrintSaleZhuHang(salemerchInfo);//aori optimize 2011-10-13 14:53:19 逐行打印//aori change 5/10/2011 9:18:37 AM proj2011-5-3-1:逐行打印完善：重输会员号、 取消商品2
// 			}
// 			pSaleDlg->m_nInputCount=oldandnewTOTALquantity-m_pLimitInfo->LimitLEFTcount;//aori del 2011-4-26 15:37:07 del test m_nSameMerchContinueInputCount
// 		}
// 		//增加正常merchinfo即超出限购数后的merchinfo
// 		pSaleDlg->m_nCurSID = AddUPLimitsalemerch(&salemerchInfo,salemerchInfo->fSaleQuantity==0?FALSE:TRUE);//函数功能:增加新的正常商品merchinfo。返回后salemerchInfo指向新的正常商品merchinfo，返回值赋给m_nCurSID，然后更新nsalecount
// 		YYquantity=oldandnewTOTALquantity-m_pLimitInfo->LimitLEFTcount;//更新saledlg的nSaleCount 此nSaleCount是新正常商品的，用于在modify正常商品的
// 		*ppsalemerchInfo=salemerchInfo;
// 		m_pLimitInfo->LimitLEFTcount=0;
// 	}
// }
void CSaleImpl::IsLimitSale(SaleMerchInfo* merchInfo){//aori add
	if(IsMember()&&merchInfo->LimitStyle>LIMITSTYLE_NoLimit&&merchInfo->LimitStyle<LIMITSTYLE_UPLIMIT
 		|| !IsMember()&&LIMITSTYLE_ALLXIAOPIAO==merchInfo->LimitStyle
 		|| LIMITSTYLE_UPLIMIT <= merchInfo->LimitStyle////aori add 2010-12-31 17:13:33 需增加限购类型：5：会员超限 6：非会员超限 7：会员全体超限
	){
		merchInfo->bLimitSale=true;
	}
	else 
		merchInfo->bLimitSale=false;
	merchInfo->bMember=IsMember();
}

//void CSaleImpl::PrintBillItemSmall(CString item_no, CString item_name, double qty, double price, double amnt)//aori add 支持限购、正常商品的区分打印
void CSaleImpl::PrintBillItemSmall(const SaleMerchInfo *pmerchinfo, double qty)
{//aori add 支持限购、正常商品的区分打印
	if (!GetSalePos()->GetPrintInterface()->m_bValid) 
		return;
	SaleMerchInfo saleInfo=*pmerchinfo;
	saleInfo.nSaleCount	=qty;
	saleInfo.fSaleQuantity=qty;
	saleInfo.fSaleAmt	=saleInfo.fSaleAmt_BeforePromotion=qty*saleInfo.fActualPrice;//aori add 2012-2-7 16:11:04 proj 2.79 测试反馈：条件促销时 不找零 加 fSaleAmt_BeforePromotion

	if(m_ServiceImpl.HasSVMerch())
		GetSalePos()->GetPrintInterface()->nPrintSalesItem_m(&saleInfo,PBID_SERVICE_RECEIPT); 
	else
		GetSalePos()->GetPrintInterface()->nPrintSalesItem_m(&saleInfo,PBID_TRANS_RECEIPT); 

	return;
}


//aori add
void CSaleImpl::SortSaleMerch(){//aori add
	if(!GetSalePos()->GetParams()->m_nBillPrintOrder)
		return;
	vector<SaleMerchInfo>::iterator merch_i,merch_k;
	SaleMerchInfo tmpmerch("");
	for (  merch_i = m_vecSaleMerch.begin(); merch_i != m_vecSaleMerch.end(); ++merch_i){
		for (  merch_k = merch_i+1; merch_k != m_vecSaleMerch.end(); ++merch_k){	
			if(GetLastActualPrice(merch_k)>GetLastActualPrice(merch_i)){
				tmpmerch=*merch_k;
				*merch_k=*merch_i;
				*merch_i=tmpmerch;
			}
		}
	}
}
double CSaleImpl::GetLastActualPrice(SaleMerchInfo* merchInfo){//aori add
	double fSaleQuantity=0.0f,fActualPrice=0.0f;
	if (merchInfo->nMerchState != RS_CANCEL)
		merchInfo->OutPut_Quantity_Price(fSaleQuantity,  fActualPrice);
	return fActualPrice;
}
// void CSaleImpl::UpdatelimitInfo(SaleMerchInfo salemerchInfo){
// 	bool bLocalExist=false;
// 	for(std::vector<LimitSaleInfo>::iterator iterLimInfo = m_VectLimitInfo.begin();iterLimInfo!=m_VectLimitInfo.end();iterLimInfo++){
// 		if(strcmp(iterLimInfo->szPLU,salemerchInfo.szPLU)==0
// 			&& (salemerchInfo.LimitStyle<LIMITSTYLE_UPLIMIT)//
// 			){//
// 			iterLimInfo->LimitLEFTcount+=salemerchInfo.nSaleCount;
// 			bLocalExist=true;
// 		}
// 	}	
// }

// void CSaleImpl::ReGeneralLimitSaleInfosLeftCount(){
// 	m_VectLimitInfo.clear();
// 	vector<SaleMerchInfo>::iterator merchInfo = NULL;
// 	for ( merchInfo = m_vecSaleMerch.begin(); merchInfo != m_vecSaleMerch.end(); ++merchInfo ) {
// 		if( !merchInfo->bLimitSale
// 			||merchInfo->nMerchState==RS_CANCEL
// 			|| LIMITSTYLE_UPLIMIT <= merchInfo->LimitStyle)
// 			continue;
// 		FindLimitInfo(merchInfo);
// 		if( merchInfo->nSaleCount <= m_pLimitInfo->LimitLEFTcount){//新增的销量在可限购范围内,不需新增正常merchinfo直接返回,更新本地limitinfo
// 			m_pLimitInfo->LimitLEFTcount -= merchInfo->nSaleCount;
// 		}else
// 			m_pLimitInfo->LimitLEFTcount=0;
// 	}
// }
void CSaleImpl::HideBankCardNum(const PayInfo* pay,TPaymentItem& pPaymentItem)
{
	//aori add 银行卡及银川icbc及银川储值卡 打印隐蔽倒数第5至倒第8位
	//Modified by liaoyu 2013-7-12 : 卡号长度>=16, 保留前6位，后4位，屏蔽中间字符
// 	if(pay->nPSID==GetSalePos()->GetParams()->m_nBankAppPSID 
// 		|| strcmp(m_PayImpl.GetPaymentStyle(pay->nPSID)->szInterfaceName,"ICBC_YC.dll") == 0  
// 		|| strcmp(m_PayImpl.GetPaymentStyle(pay->nPSID)->szInterfaceName,"ICBC_XINLI.dll") == 0  
// 		|| strcmp(m_PayImpl.GetPaymentStyle(pay->nPSID)->szInterfaceName,"ValuesCardPay_YC.dll") == 0 
// 		|| strcmp(m_PayImpl.GetPaymentStyle(pay->nPSID)->szInterfaceName,"CCB_YC.dll") == 0 
// 		|| pay->nPSID==GetSalePos()->GetParams()->m_nBankAuxPsid )
// 	{ 
// 
// 		CString cardnum(pPaymentItem.szPaymentNum);
// 		int cardNoLength=cardnum.GetLength();
// 		if(cardNoLength>8) 
// 		{
// 			int hindLength=4;
// 			if(cardNoLength>=16)
// 				hindLength=cardNoLength-10;
//   				
// 			for(int i=0;i<hindLength;i++){
// 				cardnum.SetAt(cardnum.GetLength()-1-4-i,'*');
// 			}
// 			strcpy(pPaymentItem.szPaymentNum,cardnum.GetBuffer(cardnum.GetLength()+1));
// 		}
// 		cardnum.ReleaseBuffer();
// 	 
// 	}
// 	else if(strcmp(m_PayImpl.GetPaymentStyle(pay->nPSID)->szInterfaceName,"PosMemberNEW.dll") == 0 )
// 	{
// 		int hindLength=4;
// 		CString cardnum(pPaymentItem.szPaymentNum);
// 		int cardNoLength=cardnum.GetLength();
// 		if(cardNoLength>hindLength)
// 		{
// 			for(int i=0;i<hindLength;i++){
// 				cardnum.SetAt(cardNoLength-1-i,'*');
// 			}
// 			strcpy(pPaymentItem.szPaymentNum,cardnum.GetBuffer(cardnum.GetLength()+1));
// 		}
// 		cardnum.ReleaseBuffer();
// 	}

	HideBankCardNumBNQ(pay->nPSID,pPaymentItem.szPaymentNum);
}

void CSaleImpl::HideBankCardNumBNQ(int nPSID,const char * szPaymentNum,int nFrom,int nTo)
{	
	if ( nPSID == GetSalePos()->GetParams()->m_nBankAppPSID 
		|| nPSID == GetSalePos()->GetParams()->m_nInstallmentPSID )
	{
		CString cardnum(szPaymentNum);
	
		int nLen=cardnum.GetLength();
		int nRRemainLen = nFrom-1;
		int nHide = 0;
	
		if( nLen >= nFrom && nLen <= nTo ) 
		{
			nHide = nLen - nFrom + 1 ;
		}
		else if( nLen > nTo )
		{
			nHide = nTo - nFrom + 1;			
		}
		else
		{
			nHide = 0;
		}
		
		for( int i= 0; i < nHide ; i++ )
		{
			cardnum.SetAt(cardnum.GetLength()-nFrom-i,'*');
		}
		
		strcpy((char*)szPaymentNum,cardnum.GetBuffer(cardnum.GetLength()+1));
		cardnum.ReleaseBuffer();
	}
}

void CSaleImpl::PrintCancelItem(SaleMerchInfo saleInfo){//aori add 2011-5-3 11:36:57 proj2011-5-3-1:逐行打印完善：重输会员号、 取消商品
	saleInfo.fSaleAmt=saleInfo.fSaleAmt_BeforePromotion=-saleInfo.fSaleQuantity/*nSaleCount*/*saleInfo.fActualPrice;//aori add 2012-2-7 16:11:04 proj 2.79 测试反馈：条件促销时 不找零 加 fSaleAmt_BeforePromotion
	saleInfo.fSaleQuantity=-saleInfo.fSaleQuantity/*nSaleCount*/;
	if(m_ServiceImpl.HasSVMerch())
		GetSalePos()->GetPrintInterface()->nPrintSalesItem_m(&saleInfo,PBID_SERVICE_RECEIPT);
	else
		GetSalePos()->GetPrintInterface()->nPrintSalesItem_m(&saleInfo,PBID_TRANS_RECEIPT);
}
void CSaleImpl::ReprintAll(){//aori add 5/10/2011 9:18:37 AM proj2011-5-3-1:逐行打印完善：重输会员号、 取消商品2
	if (GetSalePos()->m_params.m_nPrinterType==2){
		GetSalePos()->GetPrintInterface()->nPrintLine_m( "      ***以上小票作废***\n\n\n\n",PBID_TRANS_RECEIPT);//GetSalePos()->GetPrintInterface()->nPrintLine( "      ***以上小票作废***\n\n\n\n" );2011-12-30 14:42:38 proj 2011-12-22-1 combine 2.75 with 2.71 saleimpl
		PrintBillHeadSmall();//打印小票台头 	
		for ( vector<SaleMerchInfo>::iterator merchInfo = m_vecSaleMerch.begin(); merchInfo != m_vecSaleMerch.end(); ++merchInfo )
		{
			if ( merchInfo->nMerchState != RS_CANCEL)
			   ((CPosApp *)AfxGetApp())->GetSaleDlg()->PrintSaleZhuHang(merchInfo,true);
		}
	}	

}
bool CSaleImpl::m_bDuringLock_saleitemtemp=false;
void CSaleImpl::lock_saleitem_temp()
{	return;//aori add proj 2011-8-3-1 销售不平 2011-10-31 15:42:43 修改逻辑 绕过不平问题，取消不平日志
	HANDLE hhandle=CreateMutex(NULL,0,"bDuringLock_saleitemtemp");//aori add 2012-2-17 16:27:19 proj 2012-2-13 折扣标签校验 去掉日志跨进程锁
	WaitForSingleObject(hhandle, INFINITE);
	if(m_bDuringLock_saleitemtemp)
		CUniTrace::DumpMiniDump("已经lock了不该出现此情况 ");//这个信息非常不应该出现
	m_bDuringLock_saleitemtemp=true;
}
void CSaleImpl::unlock_saleitem_temp()
{	return;//aori add proj 2011-8-3-1 销售不平 2011-10-31 15:42:43 修改逻辑 绕过不平问题，取消不平日志
	HANDLE hhandle=CreateMutex(NULL,0,"bDuringLock_saleitemtemp");
	WaitForSingleObject(hhandle, INFINITE);
	if(!m_bDuringLock_saleitemtemp)
		CUniTrace::DumpMiniDump("已经解锁了不该出现此情况" );//这个信息非常不应该出现
	m_bDuringLock_saleitemtemp=false;
	ReleaseMutex(hhandle);
	ReleaseMutex(hhandle);
}
void CSaleImpl::check_lock_saleitem_temp()
{	return;//aori add proj 2011-8-3-1 销售不平 2011-10-31 15:42:43 修改逻辑 绕过不平问题，取消不平日志
	HANDLE hhandle=CreateMutex(NULL,0,"bDuringLock_saleitemtemp");
	int state=WaitForSingleObject(hhandle, 0);	int eee=0;
	switch(state)
	{
		case WAIT_FAILED:
			eee=GetLastError();
			CUniTrace::Trace(eee,"proj 2011-8-3-1 销售不平!!!check_lock_saleitem_temp  waitforsingle failed");
			break;
		case WAIT_ABANDONED:
			CUniTrace::Trace("proj 2011-8-3-1 销售不平!!!check_lock_saleitem_temp  wait abandoned");
			break;
		case WAIT_TIMEOUT:
			CUniTrace::DumpMiniDump("proj 2011-8-3-1 销售不平 确实有自动线程在更改temp表");
			break;
		case WAIT_OBJECT_0:
			if(m_bDuringLock_saleitemtemp)
				CUniTrace::DumpMiniDump("proj 2011-8-3-1 销售不平在不该改temp表的时候改了");
			ReleaseMutex(hhandle);
			break;
	}
}

void CSaleImpl::GeneralYinHuaInfo() //aori add 2011-8-30 10:13:21 proj2011-8-30-1 2011-8-30 10:13:21 印花需求
{
	CPosClientDB *pDB = CDataBaseManager::getConnection(true);
	for ( vector<SaleMerchInfo>::iterator merchInfo = m_vecSaleMerch.begin(); merchInfo != m_vecSaleMerch.end(); ++merchInfo) 
	{
		if (merchInfo->nMerchState != RS_CANCEL){
			//merchInfo->StampCount=(merchInfo->StampRate*merchInfo->nSaleCount)/1;
			merchInfo->StampCount=(merchInfo->StampRate*merchInfo->fSaleQuantity/*nSaleCount*/+0.0005)/1;
			CString strSQL;
			strSQL.Format("Update SaleItem_Temp set StampCount=%d "
			"WHERE SaleDT = '%s' AND HumanID = %d and ItemID = %d ", 
			merchInfo->StampCount,
			GetFormatDTString(m_stSaleDT), 
			GetSalePos()->GetCasher()->GetUserID(),merchInfo-m_vecSaleMerch.begin());
			if ( pDB->Execute2(strSQL) == 0)CUniTrace::Trace("gengeral yinhua fail");
		}
	}	
	CDataBaseManager::closeConnection(pDB);//aori add debug:2011-10-6 11:23:54 proj 2011-10-6-1 2011-10-6 11:24:01 bug: 新版本较频繁出现程序死机（不响应的情况）
}

//<--##服务调用接口##
//提交服务
bool CSaleImpl::CallService()
{
	//if(!m_ServiceImpl.NeedCallService())
	//	return true;

	CString msg="";
	char rtnMsg[256];
	int rtn = m_ServiceImpl.CallService(this,rtnMsg);
	/*
	if(rtn==SV_CALL_RETURN_SUCCESS)
	{
		msg.Format("服务提交成功!\n%s",rtnMsg);
		AfxMessageBox(msg);
	}
	else*/
	if(rtn==SV_CALL_RETURN_TIMEOUT)
	{
		msg.Format("服务提交超时,未返回确认信息!\n%s\n请打客服电话确认服务成功!",rtnMsg);
		AfxMessageBox(msg);
	}
	else
	if(rtn==SV_CALL_RETURN_FAIL)
	{
		//有不可取消的支付方式？
		if( !m_PayImpl.CanAllPaymentBeDel()  ) 
		{
			msg.Format("提交服务失败!\n%s\n因已使用了不可取消的支付方式,无法取消该笔交易.\n请联系客服进行处理!",rtnMsg);
			AfxMessageBox(msg);
		}
		else
		{
			msg.Format("提交服务失败!\n%s",rtnMsg);
			AfxMessageBox(msg);
			return false;
		}
	}
	return true;
}
//-->
CString CSaleImpl::get_xml_param(CString paramStr,CString param)
{ 
	CMarkup outxml=(CMarkup)paramStr;
	outxml.ResetMainPos();
	
	//CString strTagName = _T("");
	//CString strData = _T("");
	CString strData1 = _T("");

	//返回
	while (outxml.FindChildElem(param))
	{
		outxml.IntoElem();
		//strTagName = outxml.GetTagName(); 
		//strData = outxml.GetElemContent(); 
		strData1 = outxml.GetData();  
		outxml.OutOfElem();
	}
	outxml.ResetMainPos();
	return strData1;
}
/*
void CSaleImpl::Get_Member_input()
{
   	CString member,cardinput,cardfaceno;
	int card_type;
	member.Format("%s",m_MemberImpl.m_Member.szMemberCode);
	if (GetSalePos()->GetPosMemberInterface()->Cardinno.GetLength() > 1)
	{
		cardinput = GetSalePos()->GetPosMemberInterface()->Cardinno;
		card_type = 0;

		int indexs=cardinput.Find("=",0);
		if (indexs > 0)
		{
			cardfaceno=cardinput.Left(indexs);//会员号,去掉=后面的东西
		}
		else
		{
			cardfaceno = cardinput.Left(18);//去前18位
		}
	 
	}
	else
	{
		cardinput = member;
		card_type = 1;
		cardfaceno = member.Left(18);

	}
	cardtype = card_type;
	MemCardNO = cardinput;
	MemberCode= cardfaceno;

}
*/

//查询会员
bool CSaleImpl::QueryMember(bool *pDiffMember)
{
	m_MemberImpl.m_bMemberCardCancel = false;
	bool success=  m_MemberImpl.QueryMember(pDiffMember);
	m_bSaleCardCancel = m_MemberImpl.m_bMemberCardCancel;
	if(success && *pDiffMember)
	{
		//存储卡号
		CPosClientDB* pDB = CDataBaseManager::getConnection(true);
		if ( !pDB ) {
			CUniTrace::Trace("CMemberImpl::QueryMember() 存储会员卡号，无法获取本地数据库连接!");
			CUniTrace::Trace(MSG_CONNECTION_NULL, IT_ERROR);
			return false; 
		}
		CString strSQL;  
		strSQL.Format("UPDATE Sale_Temp SET MemberCode= '%s' ,  MemCardNO = '%s' , MemCardNOType = %d ",
			m_MemberImpl.m_Member.szMemberCode,m_MemberImpl.m_Member.szInputNO,m_MemberImpl.m_Member.nCardNoType);
		pDB->Execute2(strSQL);
		CDataBaseManager::closeConnection(pDB);
	}
	return success;
}

bool CSaleImpl::QueryMember(bool *pDiffMember, CString strMemberNO)
{
	bool diffMember = (strcmp(m_MemberImpl.m_Member.szInputNO,strMemberNO)!=0); //不同会员
	*pDiffMember=diffMember;

	//查询卡信息
	bool success = false;
	if (GetSalePos()->GetParams()->m_nUseRemoteMemScore == VIPCARD_QUERYMODE_MEMBERPLATFORM )
		success = m_MemberImpl.QueryMember(strMemberNO,VIPCARD_NOTYPE_ID); 
	else
		success = m_MemberImpl.QueryMember(strMemberNO,VIPCARD_NOTYPE_FACENO); 
	if(success)
	{
		CString msg;
		msg.Format("会员卡查询成功 卡面号:%s ",strMemberNO);
		CUniTrace::Trace(msg); 
	}

	if(success && *pDiffMember)
	{
		//存储卡号
		CPosClientDB* pDB = CDataBaseManager::getConnection(true);
		if ( !pDB ) {
			CUniTrace::Trace("CMemberImpl::QueryMember() 存储会员卡号，无法获取本地数据库连接!");
			CUniTrace::Trace(MSG_CONNECTION_NULL, IT_ERROR);
			return false; 
		}
		CString strSQL;  
		strSQL.Format("UPDATE Sale_Temp SET MemberCode= '%s' ,  MemCardNO = '%s' , MemCardNOType = %d ",
			m_MemberImpl.m_Member.szMemberCode,m_MemberImpl.m_Member.szInputNO,m_MemberImpl.m_Member.nCardNoType);
		pDB->Execute2(strSQL);
		CDataBaseManager::closeConnection(pDB);
	}
	return success;
}
//清除会员信息
//void CSaleImpl::ClearMember()
//{
//	m_MemberImpl.Clear(); 
//}
//获取会员信息
TMember CSaleImpl::GetMember()
{
	return m_MemberImpl.m_Member;
}

//查询交易应付金额(从数据库Sale_temp表)
double CSaleImpl::GetSaleTotAmtFromDB()
{
	double totAmt=0;
	CString strSQL;
	CRecordset* pRecord = NULL;
	CDBVariant var;
	
	CPosClientDB *pDB = CDataBaseManager::getConnection(true);
	if ( !pDB )
		return totAmt;
	
	try 
	{
		strSQL.Format("SELECT isnull(sum(case when oldSaleAmt is null then SaleAmt else oldSaleAmt end),0) FROM SaleItem_temp Where merchstate=0 and HumanID=%d", GetSalePos()->GetCasher()->GetUserID());
		if ( pDB->Execute(strSQL, &pRecord) > 0 ) 
		{
			pRecord->GetFieldValue((short)0, var);
			totAmt =atof(*var.m_pstring);
		}
		CDataBaseManager::closeConnection(pDB);
	} 
	catch( CException * e ) {
		CUniTrace::Trace(*e);
		e->Delete( );
		CDataBaseManager::closeConnection(pDB);
	}
	return totAmt;
}

void CSaleImpl::PrintPoolClothesMerch()
{
	PrintPoolClothesMerch(&m_vecSaleMerch);
}

//获取服饰联营大码&销售金额
void CSaleImpl::GetPoolClothesMerch(vector<SaleMerchInfo> *pVecSaleMerch,map<CString, double>* mapPoolClothesMerch)
{
	if (!GetSalePos()->GetParams()->m_bPoolClothesMerch) 
		return;

	CString tmp, merchCode;
	double saleAmt=0;
	for ( vector<SaleMerchInfo>::iterator merchInfo = pVecSaleMerch->begin(); merchInfo != pVecSaleMerch->end(); ++merchInfo)
	{
		if (merchInfo->nMerchState != RS_CANCEL)
		{
			merchCode = GetPoolSKU(merchInfo->szSimpleName);
			saleAmt = merchInfo->fSaleAmt;
			map<CString, double>::iterator itr = mapPoolClothesMerch->find(merchCode);

			if ( itr != mapPoolClothesMerch->end() ) 
				itr->second += saleAmt;
			else
				mapPoolClothesMerch->insert(map<CString, double>::value_type(merchCode, saleAmt));
		}


	/*	bool exist=false; 
		for (itr1 = mapPoolClothesMerch.begin(); itr1 != mapPoolClothesMerch.end(); itr1++)
		{
			if (merchCode == itr1->first)
			{
				itr1->second += saleAmt;
				exist=true;
				break;
			}
		}
		if(!exist)
			mapPoolClothesMerch.insert(map<CString, double>::value_type(merchCode, saleAmt));*/
	}

}

//打印服饰联营大码销售金额
void CSaleImpl::PrintPoolClothesMerch(vector<SaleMerchInfo> *pVecSaleMerch)
{
	if (!GetSalePos()->GetParams()->m_bPoolClothesMerch) 
		return;

	map<CString, double> mapPoolClothesMerch;
	map<CString, double>::iterator itr;

	GetPoolClothesMerch(pVecSaleMerch,&mapPoolClothesMerch);
	GetSalePos()->GetPrintInterface()->nPrintHorLine();
	sprintf(m_szBuff, "联营大码金额合计:");
	GetSalePos()->GetPrintInterface()->nPrintLine(m_szBuff);	
 
	for (itr = mapPoolClothesMerch.begin(); itr != mapPoolClothesMerch.end(); itr++)
	{
		sprintf(m_szBuff,"%s                %.2f", itr->first, itr->second);
		GetSalePos()->GetPrintInterface()->nPrintLine(m_szBuff);
	}

	GetSalePos()->GetPrintInterface()->nPrintHorLine();
 
}

//获取服饰商品联营大码SKU
CString CSaleImpl::GetPoolSKU(CString pMerchName)
{
	CString sku;
	sku.Format(_T(""));
	if (pMerchName.Left(1) == "*")
		sku = pMerchName.Mid(1, 6);
	else
		sku = pMerchName.Left(6);
	return sku;
}
//======================================================================
// 函 数 名：QueryEorderMemberCode
// 功能描述：查询商铺的电商会员号
// 输入参数：
// 输出参数：bool
// 创建日期：2015-3-2 
// 修改日期：
// 作  者： LiuBW
// 附加说明：
//======================================================================
bool CSaleImpl::QueryEorderMemberCode(bool* pDiffMember)
{
	CString EOrderMemberCode = GetSalePos()->m_params.m_EOrderMemberCode;
	bool diffMember=false;
	diffMember = (strcmp(m_MemberImpl.m_Member.szInputNO,EOrderMemberCode.GetBuffer(0))!=0);//不同会员
	EOrderMemberCode.ReleaseBuffer();
	*pDiffMember=diffMember;

	bool success=  m_MemberImpl.QueryMember(EOrderMemberCode.GetBuffer(0),VIPCARD_NOTYPE_FACENO);//查询店铺电商结算会员号
	EOrderMemberCode.ReleaseBuffer();
	if(success && *pDiffMember)
	{
		//存储卡号
		CPosClientDB* pDB = CDataBaseManager::getConnection(true);
		if ( !pDB ) {
			CUniTrace::Trace("CMemberImpl::QueryMember() 存储会员卡号，无法获取本地数据库连接!");
			CUniTrace::Trace(MSG_CONNECTION_NULL, IT_ERROR);
			return false; 
		}
		CString strSQL;  
		strSQL.Format("UPDATE Sale_Temp SET MemberCode= '%s' ,  MemCardNO = '%s' , MemCardNOType = %d ",
			m_MemberImpl.m_Member.szMemberCode,m_MemberImpl.m_Member.szInputNO,m_MemberImpl.m_Member.nCardNoType);
		pDB->Execute2(strSQL);
		CDataBaseManager::closeConnection(pDB);
	}
	return success;
}

//判断该笔交易是否为电商拣货
bool CSaleImpl::IsEOrder()
{
	if(m_EorderID!=""&&m_EMemberID=="")
		return true;
	else 
		return false;
}

//判断该笔交易是否为电商会员结算
bool CSaleImpl::IsEMSale()
{	
	if(m_EMemberID!="")
		return true;
	else 
		return false;
}

void CSaleImpl::Upd_EMorder_status(int status,int type,bool Downgradeflag)
{
	if (!Downgradeflag)
	{
		CPosClientDB* pDB = CDataBaseManager::getConnection();
		if ( !pDB )
			return;

		CString strSQL,strBuf;
		CString saledt = GetSalePos()->GetTableDTExt(&m_stSaleDT);
		try
		{
			if (type == 1)//支付后保存
				strSQL.Format("update sale%s set addcol8 = '%d'  " 
						" WHERE  saledt >= GETDATE()-2 AND SaleID = %d  ",
						saledt,status , GetSaleID());
			else
				strSQL.Format("update sale_temp set EMOrderStatus = %d  ",status );
			GetSalePos()->GetWriteLog()->WriteLog(strSQL,3);//日志
			pDB->Execute2(strSQL);
			m_EMOrderStatus=status;
			strBuf.Format("更新电商订单状态数据成功！");
			GetSalePos()->GetWriteLog()->WriteLog(strSQL,3);//日志

		}catch(CException *e) {
			CUniTrace::Trace(*e);
			e->Delete();
			strBuf.Format("更新电商订单状态数据失败！");
			GetSalePos()->GetWriteLog()->WriteLog(strSQL,3);//日志

		}
		CDataBaseManager::closeConnection(pDB);
	}
}

void CSaleImpl::Upd_EMorder_EOrderID(CString m_EOrderID,int type )
{
	CPosClientDB* pDB = CDataBaseManager::getConnection();
	if ( !pDB )
		return;

	CString strSQL,strBuf;
	CString saledt = GetSalePos()->GetTableDTExt(&m_stSaleDT);
	try
	{
		if (type == 1)//支付后保存 
			strSQL.Format("update sale%s set addcol6 = '%s'   " 
					" WHERE  saledt >= GETDATE()-2 AND SaleID = %d  ",
					saledt,m_EorderID.GetBuffer(0) , GetSaleID());
		else
			strSQL.Format("UPDATE Sale_Temp SET EOrderID = '%s' ",m_EorderID.GetBuffer(0));

		GetSalePos()->GetWriteLog()->WriteLog(strSQL,3);//日志
		pDB->Execute2(strSQL);
		strBuf.Format("更新电商订单状态数据成功！");
		GetSalePos()->GetWriteLog()->WriteLog(strSQL,3);//日志

	}catch(CException *e) {
		CUniTrace::Trace(*e);
		e->Delete();
		strBuf.Format("更新电商订单状态数据失败！");
		GetSalePos()->GetWriteLog()->WriteLog(strSQL,3);//日志

	}
	CDataBaseManager::closeConnection(pDB);
}

BOOL CSaleImpl::OrderIsHas(CString strOrderID)
{
	for(int n = 0; n < (int)m_vecSaleMerch.size(); n++)
	{
		if(m_vecSaleMerch[n].nItemType != 2)
			continue;
		CString strOrderNoOld = m_vecSaleMerch[n].strOrderNo;
		if(strOrderNoOld.IsEmpty() || m_vecSaleMerch[n].nMerchState == RS_CANCEL)
			continue;
		if(strOrderNoOld.Compare(strOrderID) == 0)
		{
			return TRUE;	
		}
	}
	return FALSE;
}
//返回整单的标价
bool CSaleImpl::AddOrderItem(CString strOrderID, double &dTotalAmt)
{
	GetSalePos()->m_Task.pauseAutoTask();
	CStringArray strArr;
	GetSalePos()->m_downloader.GetOrderItemInfo(strOrderID, strArr);
	GetSalePos()->m_Task.resumeAutoTask();

	CPosClientDB* pDB = CDataBaseManager::getConnection();
	if ( !pDB )
	{
		CUniTrace::Trace("CSaleImpl::AddOrderItem,pDB == NULL");
		return false;
	}

	CString strSQL;
	CDBVariant var;
	CRecordset *pRecord = NULL;
	CString strSKU(_T(""));
	dTotalAmt = 0;

	//检查所有的merchid是否在本地库中存在
	for(int m = 0; m < strArr.GetSize(); m++)
	{
		CString strItem = strArr.GetAt(m);
		CMarkup xml = (CMarkup)strItem;
		CString strMerchId = GetSingleParamValue(xml, "MerchId");
		if(strMerchId.IsEmpty())
		{
			CUniTrace::Trace("CSaleImpl::AddOrderItem::strMerchId.IsEmpty()");
			return false;
		}
		
		strSQL.Format("SELECT MIN(PLU) FROM SaleMerch WHERE MerchID = '%s'", strMerchId);
		if ( pDB->Execute(strSQL,&pRecord) <= 0 )
		{
			CString strErr = strSQL + _T("在本地salemerch中没有找到该商品");
			CUniTrace::Trace(strErr);
			pDB->CloseRecordset(pRecord);
			return false;
		}
	}

	for(int n = 0; n < strArr.GetSize(); n++)
	{
		CString strItem = strArr.GetAt(n);
		CMarkup xml = (CMarkup)strItem;
		CString strItemNo = GetSingleParamValue(xml, "ItemNo");
		CString strMerchId = GetSingleParamValue(xml, "MerchId");
		//CString strCondPromoDisc = GetSingleParamValue(xml, "CondPromDiscount");
		CString strAfterMarkDownTotAmt = GetSingleParamValue(xml, "SaleAmt");
		CString strDiscType = GetSingleParamValue(xml, "DiscountType");
		CString strSaleQty = GetSingleParamValue(xml, "SaleQty");
		CString strAddCol4 = GetSingleParamValue(xml, "AddCol4");
		double dSaleQty = atof(strSaleQty);

		if(strItemNo.IsEmpty() || strMerchId.IsEmpty() || dSaleQty <= 0)
			continue;

		//CAutoDB db(true);

		strSQL.Format("SELECT MIN(PLU) FROM SaleMerch WHERE MerchID = '%s'", strMerchId);
		
		try
		{
			if ( pDB->Execute(strSQL,&pRecord) > 0 )
			{
				//解决崩溃，sql执行结果为空，不能直接赋值 Modified by dandan.wu [2016-1-20]
				pRecord->GetFieldValue((short)0, var);
				
				if(var.m_dwType != DBVT_NULL && strlen(*var.m_pstring)>0)
				{
					strSKU = *var.m_pstring;
					int nDiscType = atoi(strDiscType);
					double dbTemp = 0.0;
					CString strTempAmt = _T("0.0");
					dbTemp = atof(strAddCol4)*dSaleQty;

					double dSaleAmt = (nDiscType == PROMO_STYLE_MANUALDISCOUNT)?atof(strAfterMarkDownTotAmt):dbTemp;

					//保留2位小数,否则由于精度问题有分差异，汇总失败，导致对账不平 [dandan.wu 2016-8-3]	
					strTempAmt.Format("%.2f",dSaleAmt);
					dSaleAmt = atof(strTempAmt);

					strTempAmt.Format("%.2f",dbTemp);
					dbTemp = atof(strTempAmt);

					AddItem(strSKU, Inputer_Normal, false, 0, strOrderID, strItemNo, 0, dSaleQty,dSaleAmt,nDiscType,atof(strAddCol4));
					if(!strAddCol4.IsEmpty())
						dTotalAmt += dbTemp;//atof(strAddCol4)*dSaleQty;
				}
				else
				{
					CString strErr = strSQL + _T("返回空值,是无效商品！");
					CUniTrace::Trace(strErr);
					pDB->CloseRecordset(pRecord);
					return false;
				}
			}
		}
		catch (CException* e) 
		{
			pDB->CloseRecordset(pRecord);
			CUniTrace::Trace("CSaleImpl::AddOrderItem");
			CUniTrace::Trace(*e);
			e->Delete();
			return false;
		}	
	}
	pDB->CloseRecordset(pRecord);
	if(strArr.GetSize() > 0)
	{
		CFinDataManage finDataManage;
		finDataManage.UpdateOrderStatue(strOrderID,ORDER_CHARGING);
	}
	return true;
}

void CSaleImpl::GetSaleOrderInfo(CString strOrderID, CString &strOrderInfo)
{
	GetSalePos()->m_Task.pauseAutoTask();
	GetSalePos()->m_downloader.GetOrderInfo(strOrderID, strOrderInfo);
	GetSalePos()->m_Task.resumeAutoTask();
	if(strOrderInfo.IsEmpty())
		CUniTrace::Trace("CSaleDlg::AddOrderToList:strOrderInfo.IsEmpty()");
}

//根据会员号查询订单
void CSaleImpl::GetMemberOrders(CString strMemberNo, CStringArray &strArr)
{
	CMarkup xml_param;
	xml_param.SetDoc("<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
	xml_param.AddElem("InputParameter");
	xml_param.IntoElem();
	xml_param.AddElem("type",DOWNLOAD_ORDER_BYMEMBER);
	xml_param.AddElem("MemberCode",strMemberNo);
	xml_param.OutOfElem();
	CString strXml = xml_param.GetDoc();

	GetSalePos()->m_Task.pauseAutoTask();
	GetSalePos()->m_downloader.GetNormalInfo(strXml, strArr);
	GetSalePos()->m_Task.resumeAutoTask();
}

//会员折扣
void CSaleImpl::SetMemberPrice()
{
	double fDiscountRate = 1.0/*m_MemberImpl.m_Member.dMemberDiscountRate*/;//会员折扣率
	if ( fDiscountRate != 1.0f ) 
	{
		for(int n = 0; n < (int)m_vecSaleMerch.size(); n++)
		{
			if(m_vecSaleMerch[n].bVipDiscount)
			{
				m_vecSaleMerch[n].fSaleAmt *= fDiscountRate;
			}
		}
	}
}

void CSaleImpl::SetAllOrderStatus(int nStatues)
{
	CString strOrderNoPre = "";
	for(int n = 0; n < (int)m_vecSaleMerch.size(); n++)
	{
		if(m_vecSaleMerch[n].nItemType != 2 || m_vecSaleMerch[n].nMerchState == RS_CANCEL)
		{
			strOrderNoPre = "";
			continue;
		}

		if(m_vecSaleMerch[n].strOrderNo != strOrderNoPre)
		{
			strOrderNoPre = m_vecSaleMerch[n].strOrderNo;
			CFinDataManage cFinDataManage;
			cFinDataManage.UpdateOrderStatue(strOrderNoPre, nStatues);
		}
	}
}

bool CSaleImpl::AddPromTmpInfo(int nItemID, int nPromID, CString strPromName, double dDiscount)
{
	CPosClientDB* pDB = CDataBaseManager::getConnection(true);
	if ( !pDB ) {
		CUniTrace::Trace("CSaleImpl::AddPromTmpInfo");
		CUniTrace::Trace(MSG_CONNECTION_NULL, IT_ERROR);
		return false;
	}
	
	CString strSaleDT = GetFormatDTString(m_stSaleDT);
	CString strSQL;
	strSQL.Format("insert SalePromotion_temp (SaleDT,SaleID,ItemID,PromoID,PromoName,Discount) values ('%s',%d,%d,%d,'%s',%f)",
		strSaleDT,m_nSaleID,nItemID,nPromID,strPromName, dDiscount);
	TRACE(strSQL);
	pDB->Execute2(strSQL);

	CDataBaseManager::closeConnection(pDB);
	return true;
}
//根据订单号得到posID saleID saleDT
BOOL CSaleImpl::GetSaleByOrder(CString strOrderNo, int &nPosID, int &nSaleID, CString &strSaleDT)
{
	CMarkup xml_param;
	xml_param.SetDoc("<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
	xml_param.AddElem("InputParameter");
	xml_param.IntoElem();
	xml_param.AddElem("type",DOWNLOAD_SALE_BYORDERNO);
	xml_param.AddElem("OrderNo",strOrderNo);
	xml_param.OutOfElem();
	CString strXml = xml_param.GetDoc();
	
	CStringArray strArr;
	GetSalePos()->m_Task.pauseAutoTask();
	GetSalePos()->m_downloader.GetNormalInfo(strXml, strArr);
	GetSalePos()->m_Task.resumeAutoTask();

	if(strArr.GetSize() == 1)
	{
		CString strInfo = strArr.GetAt(0);
		CString strPrompt;
		strPrompt.Format("根据订单号得到的信息:%s",strInfo);
		CUniTrace::Trace(strPrompt);
		if(strInfo.IsEmpty())
			return FALSE;
		
		CMarkup xml = (CMarkup)strInfo;
		nPosID = atoi(GetSingleParamValue(xml, "PosID"));
		nSaleID = atoi(GetSingleParamValue(xml, "SaleID"));
		strSaleDT = GetSingleParamValue(xml, "SaleDT");
		strPrompt.Format("根据订单号得到的posID:%d  saleID:%d  saleDT:%s", nPosID, nSaleID, strSaleDT);
		CUniTrace::Trace(strPrompt);

		return TRUE;
	}
	AfxMessageBox("根据订单号得不到交易信息!");
	CUniTrace::Trace("根据订单号得不到交易信息");
	return FALSE;
}

bool CSaleImpl::AddSaleItemPromtions(const vector<TSaleItemPromotion> &vecSaleItemProm)
{
	CPosClientDB* pDB = CDataBaseManager::getConnection(true);
	if ( !pDB ) {
		CUniTrace::Trace("CSaleImpl::AddSaleItemPromtions");
		CUniTrace::Trace(MSG_CONNECTION_NULL, IT_ERROR);
		return false;
	}
	
	CString strSaleDT = GetFormatDTString(m_stSaleDT);
	CString strSQL;
	for(int n = 0; n < (int)vecSaleItemProm.size(); n++)
	{
		strSQL.Format("INSERT INTO SaleItemPromotion_temp ( SaleDT,SaleID,ItemID,ItemPromID,\
			PLU,MakeOrgNO,PromoID,PromoStyle,PromoName,DiscountRate,Discount,PromoServiceID)\
			VALUES  ( '%s',%d,%d,%d,'%s',%d,'%s',%d,'%s',%f,%f,'%s')",
			strSaleDT, m_nSaleID, vecSaleItemProm[n].nItemID, n+1, vecSaleItemProm[n].strPlu,
			vecSaleItemProm[n].nMakeOrgN0, vecSaleItemProm[n].strPromoID, vecSaleItemProm[n].nPromoStyle,
			vecSaleItemProm[n].strPromoName, vecSaleItemProm[n].dDiscountRate,vecSaleItemProm[n].dDiscount,
			vecSaleItemProm[n].strPromoSerciveID);
		pDB->Execute2(strSQL);

		CUniTrace::Trace(strSQL);
	}

	CDataBaseManager::closeConnection(pDB);
	return true;
}

double CSaleImpl::GetItemSaleCount(int nSID)
{
	if ( nSID >= m_vecSaleMerch.size() || nSID < 0 ) 
		return 0;
	SaleMerchInfo *merchInfo = &m_vecSaleMerch[nSID];
	if ( merchInfo->fSaleQuantity/*nSaleCount*/ == 0 )
		return 0;
	double dCount = (merchInfo->nItemType == ITEM_IN_ORDER) ? 1 : merchInfo->fSaleQuantity/*nSaleCount*/;
	return dCount;
} 

//markdown后是否需要新加一行
int CSaleImpl::AddItemForMarkDown(double dCount, int nSID)
{
	if ( nSID >= m_vecSaleMerch.size() || nSID < 0 ) 
		return nSID;
	SaleMerchInfo *merchInfo = &m_vecSaleMerch[nSID];

	//数量记在fSaleQuantity中 [dandan.wu 2016-4-26]
	if ( merchInfo->fSaleQuantity/*nSaleCount*/ == 0 )
		return nSID;
	if( dCount < merchInfo->fSaleQuantity/*nSaleCount*/)
	{
		//拆零商品笔数不变 [dandan.wu 2016-4-26]
		if ( merchInfo->nMerchType == OPEN_STOCK_2 || merchInfo->nMerchType == OPEN_STOCK_5 )
		{
			merchInfo->nSaleCount = merchInfo->nSaleCount;
		}
		else
		{
			merchInfo->nSaleCount -=  (int)dCount;
		}	
		merchInfo->fSaleQuantity -= dCount;

		//重新计算fSaleAmt
		if(merchInfo->IncludeSKU>1)
			merchInfo->fSaleAmt = merchInfo->fActualBoxPrice * merchInfo->fSaleQuantity/*nSaleCount*//merchInfo->IncludeSKU;			
		else
			merchInfo->fSaleAmt = merchInfo->fActualPrice * merchInfo->fSaleQuantity/*nSaleCount*/;			
		merchInfo->fSaleAmt_BeforePromotion = merchInfo->fSaleAmt; 

		SaleMerchInfo newMerchInfo = m_vecSaleMerch[nSID];

		//拆零商品新增一行，笔数为1 [dandan.wu 2016-4-26]
		if ( newMerchInfo.nMerchType == OPEN_STOCK_2 || newMerchInfo.nMerchType == OPEN_STOCK_5 )
		{
			newMerchInfo.nSaleCount = 1;
		}
		else
		{
			newMerchInfo.nSaleCount =  (int)dCount;
		}	
		newMerchInfo.fSaleQuantity = dCount;
		newMerchInfo.nSID = nSID+1;

		//重新计算fSaleAmt
		if(newMerchInfo.IncludeSKU>1)
			newMerchInfo.fSaleAmt = newMerchInfo.fActualBoxPrice * dCount/newMerchInfo.IncludeSKU;			
		else
			newMerchInfo.fSaleAmt = newMerchInfo.fActualPrice * dCount;			
		newMerchInfo.fSaleAmt_BeforePromotion = newMerchInfo.fSaleAmt; 

		return AddItem(newMerchInfo);
	}
	return nSID;
}

BOOL CSaleImpl::PrintBillTotalMiya(bool bIsEndSign)
{
	bool bFind = false;
	for ( vector<PayInfo>::const_iterator itpay = m_PayImpl.m_vecPay.begin(); itpay != m_PayImpl.m_vecPay.end(); ++itpay) 
	{
		if( itpay->nPSID == GetSalePos()->GetParams()->m_nAliPayPSID 
			|| itpay->nPSID == GetSalePos()->GetParams()->m_nWeChatPayPSID)
		{
			bFind = true;
			break;
		}
	}
	
	if ( !bFind )
	{
		return FALSE; 
	}
	
	bool bInvoicePrint = false;
	bInvoicePrint = GetSalePos()->m_params.m_bAllowInvoicePrint;
	
	//打印明细单
	if(!GetSalePos()->GetPrintInterface()->m_bValid)
		return FALSE;
	BOOL bNeedSign = FALSE;
	
	CPrintInterface  *printer = GetSalePos()->GetPrintInterface();
	
	//打印横线
	if ( !bInvoicePrint)
	{
		GetSalePos()->GetPrintInterface()->nPrintHorLine();
	}
	
	const TPaymentStyle *ps = NULL;


	
	//打印交易头信息
	if (bIsEndSign)
	{
		//切纸
		GetSalePos()->GetPrintInterface()->nEndPrint_m();
		GetSalePos()->GetWriteLog()->WriteLog("CSaleImpl::PrintBillBootomSmall(),切纸",POSLOG_LEVEL1);

		sprintf(m_szBuff, "日期:%04d-%02d-%02d %02d:%02d 机号:%03d", 
			m_stSaleDT.wYear, m_stSaleDT.wMonth, m_stSaleDT.wDay, m_stSaleDT.wHour, m_stSaleDT.wMinute, m_pParam->m_nPOSID);
		printer->nPrintLine(m_szBuff);
		
		sprintf(m_szBuff, "单号:%06d 收银员:%s", 
			m_nSaleID,GetSalePos()->GetCasher()->GetUserCode());
		printer->nPrintLine(m_szBuff);
	}
	
	//打印支付方式及金额
	for ( vector<PayInfo>::const_iterator pay = m_PayImpl.m_vecPay.begin(); pay != m_PayImpl.m_vecPay.end(); ++pay) 
	{
		if(pay->nPSID != GetSalePos()->GetParams()->m_nAliPayPSID &&
			pay->nPSID != GetSalePos()->GetParams()->m_nWeChatPayPSID)
			continue;
		
		bNeedSign = TRUE;
		vector<PayInfoItem> vecFindPayItem;
		m_PayImpl.GetPayInfoItemVect(pay->nPSID, &vecFindPayItem);
		
		ps = m_PayImpl.GetPaymentStyle(pay->nPSID);
		
		if (vecFindPayItem.size() > 0)
		{
			for ( vector<PayInfoItem>::iterator payInfoItem = vecFindPayItem.begin(); payInfoItem != vecFindPayItem.end(); ++payInfoItem)
			{	
				printer->nPrintLine(" ");
				if (pay->nPSID == GetSalePos()->GetParams()->m_nAliPayPSID)
				{				
					sprintf(m_szBuff,"支付宝:%12.2f",payInfoItem->fPaymentAmt);
				}else{
					sprintf(m_szBuff,"微信:%12.2f",payInfoItem->fPaymentAmt);
				}
 				printer->nPrintLine( m_szBuff);

				char strUniqueOrderID[64+1] = {0};
				CString ExtraInfo = "";
				CString strRandom = "";
				CString strDiscount = "";
				CString strDate = "";
				SYSTEMTIME st;
				GetLocalTime(&st);
				int iSaleID = 0;
				ExtraInfo.Format("%s",payInfoItem->szExtraInfo);
				strDiscount.Format("%s",payInfoItem->szLoanType);

				if (!strDiscount.IsEmpty() && strDiscount.GetLength() >= 24)
				{
					CString strOrgDiscount = "";
					strOrgDiscount = strDiscount.Mid(0,12);
					strOrgDiscount = FormatMiyaDiscount(strOrgDiscount);
					sprintf(m_szBuff,"商户优惠金额:%s",strOrgDiscount);
					printer->nPrintLine( m_szBuff);

					CString strOtherDiscount = "";
					strOtherDiscount = strDiscount.Mid(12,12);
					strOtherDiscount = FormatMiyaDiscount(strOtherDiscount);
					sprintf(m_szBuff,"其他优惠金额:%s",strOtherDiscount);
					printer->nPrintLine( m_szBuff);
				}

				if ( !ExtraInfo.IsEmpty())
				{
					if (ExtraInfo.GetLength() >= 54)
					{
						strRandom = ExtraInfo.Mid(0,3);
						strDate = ExtraInfo.Mid(42,12);
						iSaleID = m_nSaleID;
						sprintf(strUniqueOrderID,"%12s%4d%.2d%.6d%02d%03s",strDate,
							GetSalePos()->GetParams()->m_nOrgNO,GetSalePos()->GetParams()->m_nPOSID,
							iSaleID,pay->nPSID,strRandom);
						sprintf(m_szBuff,"商户订单号:%s",strUniqueOrderID);
						printer->nPrintLine( m_szBuff);
					}	
					if (ExtraInfo.GetLength() >= 31)
					{
						CString strSysPayNum = "";
						strSysPayNum = ExtraInfo.Mid(3,28);
						sprintf(m_szBuff,"支付平台号:%s",strSysPayNum);
						printer->nPrintLine( m_szBuff);
					}
					if (ExtraInfo.GetLength() >= 42)
					{
						CString strCustomerID = "";
						strCustomerID = ExtraInfo.Mid(31,11);
						sprintf(m_szBuff,"支付账号:%s",strCustomerID);
						printer->nPrintLine( m_szBuff);
					}
					if (ExtraInfo.GetLength() < 54)
					{
						GetSalePos()->GetWriteLog()->WriteLog(ExtraInfo,4);
					}
				}				
			}
		}
	}
	
	if (bNeedSign&&!bInvoicePrint)
	{
		printer->nPrintLine(" ");
		sprintf(m_szBuff, "%s%s", "雇员签名:","....................");
		printer->nPrintLine(m_szBuff);
		printer->nPrintLine(" ");
		sprintf(m_szBuff, "%s%s", "顾客签名:","....................");
		printer->nPrintLine(m_szBuff);
		printer->nPrintLine(" ");
		printer->nPrintLine(" ");
	}
	return bNeedSign;
}

BOOL CSaleImpl::PrintBillTotalPrepaid(bool bIsEndSign)
{
	bool bFind = false;
	for ( vector<PayInfo>::const_iterator itpay = m_PayImpl.m_vecPay.begin(); itpay != m_PayImpl.m_vecPay.end(); ++itpay) 
	{
		if( itpay->nPSID == GetSalePos()->GetParams()->m_nSavingCardPSID)
		{
			bFind = true;
			break;
		}
	}

	if ( !bFind )
	{
		return FALSE; 
	}

	bool bInvoicePrint = false;
	bInvoicePrint = GetSalePos()->m_params.m_bAllowInvoicePrint;

	//打印储值卡明细单
	if(!GetSalePos()->GetPrintInterface()->m_bValid)
		return FALSE;
	BOOL bNeedSign = FALSE;

	CPrintInterface  *printer = GetSalePos()->GetPrintInterface();
	//int i=0,nMaxLen = GetSalePos()->GetParams()->m_nMaxPrintLineLen;
	//if ( GetSalePos()->GetParams()->m_nPaperType == 1 )
	//	nMaxLen = 32;	//打印横线
	//GetPrintChars(m_szBuff, nMaxLen, nMaxLen, true);
	//printer->nPrintLine( m_szBuff );

	//打印横线
	if ( !bInvoicePrint)
	{
		GetSalePos()->GetPrintInterface()->nPrintHorLine();
	}

	const TPaymentStyle *ps = NULL;


	//打印交易头信息
	if (bIsEndSign)
	{
		
		GetSalePos()->GetPrintInterface()->nEndPrint_m();
		GetSalePos()->GetWriteLog()->WriteLog("CSaleImpl::PrintBillBootomSmall(),切纸",POSLOG_LEVEL1);

		sprintf(m_szBuff, "日期:%04d-%02d-%02d %02d:%02d 机号:%03d", 
			m_stSaleDT.wYear, m_stSaleDT.wMonth, m_stSaleDT.wDay, m_stSaleDT.wHour, m_stSaleDT.wMinute, m_pParam->m_nPOSID);
		printer->nPrintLine(m_szBuff);

		sprintf(m_szBuff, "单号:%06d 收银员:%s", 
			m_nSaleID,GetSalePos()->GetCasher()->GetUserCode());
		printer->nPrintLine(m_szBuff);
	}

	//打印支付方式及金额
	for ( vector<PayInfo>::const_iterator pay = m_PayImpl.m_vecPay.begin(); pay != m_PayImpl.m_vecPay.end(); ++pay) 
	{
		if(pay->nPSID != GetSalePos()->GetParams()->m_nSavingCardPSID)
			continue;

		bNeedSign = TRUE;
		vector<PayInfoItem> vecFindPayItem;
		m_PayImpl.GetPayInfoItemVect(pay->nPSID, &vecFindPayItem);

		ps = m_PayImpl.GetPaymentStyle(pay->nPSID);
	
		if (vecFindPayItem.size() > 0)
		{
			for ( vector<PayInfoItem>::iterator payInfoItem = vecFindPayItem.begin(); payInfoItem != vecFindPayItem.end(); ++payInfoItem)
			{
				printer->nPrintLine(" ");
				sprintf(m_szBuff,"储值卡:%12.2f",payInfoItem->fPaymentAmt);
				printer->nPrintLine( m_szBuff);
				sprintf(m_szBuff,"号码:%s",payInfoItem->szPaymentNum);
				printer->nPrintLine(m_szBuff);
				sprintf(m_szBuff,"持卡人:%s",payInfoItem->szExtraInfo);
				printer->nPrintLine(m_szBuff);
				sprintf(m_szBuff,"余额:%14.2f",payInfoItem->fRemainAmt);
				printer->nPrintLine(m_szBuff);
			}
		}
	}

	if (bNeedSign&&!bInvoicePrint)
	{
		printer->nPrintLine(" ");
		sprintf(m_szBuff, "%s%s", "雇员签名:","....................");
		printer->nPrintLine(m_szBuff);
		printer->nPrintLine(" ");
		sprintf(m_szBuff, "%s%s", "顾客签名:","....................");
		printer->nPrintLine(m_szBuff);
		printer->nPrintLine(" ");
		printer->nPrintLine(" ");
	}
	return bNeedSign;
}

void CSaleImpl::InvoicePrintWholeSale(bool& bInvoiceSegmentUseUp)
{
	//打印机不可用则退出
	if (!GetSalePos()->GetPrintInterface()->m_bValid) 
	{
		GetSalePos()->GetWriteLog()->WriteLog("END CSaleImpl::InvoicePrintWholeSale PrintInterface::m_bValid禁止打印",4,POSLOG_LEVEL1); 
		return;
	} 
	
	if ( !GetSalePos()->m_params.m_bAllowInvoicePrint )
	{
		GetSalePos()->GetWriteLog()->WriteLog("END CSaleImpl::InvoicePrintWholeSale 禁止发票打印",4,POSLOG_LEVEL1); 
		return;
	}

	CUniTrace::Trace("********************************开始打印销售发票********************************");
	char szBuff[128] = {0};
	sprintf(szBuff,"(除单独打印的储值卡、支付宝/微信，销售：发票总行数:%d,总页数:%d",m_nInvoiceTotalPrintLineSale,m_nInvoiceTotalPageSale);
	CUniTrace::Trace(szBuff);

	sprintf(szBuff,"单独打印储值卡，发票总行数:%d,总页数:%d",m_nInvoiceTotalPrintLinePrepaid,m_nInvoiceTotalPagePrepaid);
	CUniTrace::Trace(szBuff);

	sprintf(szBuff,"支付宝/微信直连，发票总行数:%d,总页数:%d",m_nInvoiceTotalPrintLineAPay,m_nInvoiceTotalPageAPay);
	CUniTrace::Trace(szBuff);

	//更新下一笔交易的第一张发票号=最后一笔交易（销售、储值卡充值/提现）保存的最后一张发票号+1
	InvoiceUpdateInvoiceNO(atoi(m_strInvoiceNoFirst)+m_nInvoiceTotalPage);

	//发票头部分
	InvoicePrintHead(m_strInvoiceNoFirst,m_nInvoiceTotalPageSale);

	//现货商品
	printBillItemInStock(m_vecSaleMerch);

	//订单商品
	printBillItemInOrder(m_vecSaleMerch);

	//支付方式和找零
	InvoicePrintPayment();

	//发票打印，若支付方式中有储值卡，打印交易后，单独打印
	InvoicePrintPrepaidCard();

	//若支付宝/微信直连，单独打印
	m_InvoiceImpl.InvoicePrintAPay(m_vecPayItemAPay);

	InvoiceRest();

	//判断发票卷是否用完，若用完，提示用户换新的发票卷
	if ( m_eInvoiceRemain == INVOICE_REMAIN_EQUAL )
	{
		bInvoiceSegmentUseUp = true;

		CString strMsg = _T("发票卷用完，请更换新的发票卷！");
		
		//提示用户换发票卷
		MessageBox(NULL,strMsg, "提示", MB_OK | MB_ICONWARNING);		
		CUniTrace::Trace(strMsg);
		
		//青岛发票最后一张打印汇总信息
		if ( GetSalePos()->m_params.m_strInvoiceDistrict == INVOICE_DISTRICT_QD )
		{
			InvoicePrintSummaryInfo();
		}
		
		//更新旧发票卷的发票号
		InvoiceUpdateInvoiceNO(atoi(m_strInvoiceEndNo)+1);
	}

	CUniTrace::Trace("********************************结束打印销售发票********************************");
}

void CSaleImpl::SetInvoiceInfo(int& nSegmentID,CString& strInvoiceCode,CString& strInvoiceStartNo,CString& strInvoiceEndNo)
{
	m_nSegmentID = nSegmentID;
	m_strInvoiceCode = strInvoiceCode;
	m_strInvoiceStartNo = strInvoiceStartNo;
	m_strInvoiceEndNo = strInvoiceEndNo;
}

void CSaleImpl::InvoicePrintHead(const CString& strInvoiceNo,const int& nTotalPage)
{
	TInvoicePrintInfo stPrintHead;
	char szBuff[256] = {0};
	CString strDistrict = _T("");
	strDistrict = GetSalePos()->m_params.m_strInvoiceDistrict;
	bool bRes = false;
	
	//日期/时间
	stPrintHead.stSaleDT = m_stSaleDT;	

	//最后一笔保存的销售流水号
	stPrintHead.nSaleID = m_nSaleID;

	//会员卡号
	memcpy(stPrintHead.szMemberCode,GetMember().szMemberCode,strlen(GetMember().szMemberCode));
	
	//付款单位（会员名称）
	memcpy(stPrintHead.szMemberName,GetMember().szMemberName,strlen(GetMember().szMemberName));

	//发票所在城市简称,如青岛：QD 苏州：SZ
	sprintf(stPrintHead.szInvoiceDistrict,"%s",strDistrict);

	//发票代码
	sprintf(stPrintHead.szInvoiceCode,"%s",m_strInvoiceCode);

	//发票号
	sprintf(stPrintHead.szInvoiceNo,"%s",strInvoiceNo);

	//此笔交易需要打印的发票总页数
	 stPrintHead.nInvoiceTotalPrintLineNormal = m_nInvoiceTotalPrintLineSale;
	 stPrintHead.nInvoiceTotalPageNormal = nTotalPage;

	 stPrintHead.nInvoiceTotalPrintLinePrepaid = m_nInvoiceTotalPrintLinePrepaid;
	 stPrintHead.nInvoiceTotalPagePrepaid = m_nInvoiceTotalPagePrepaid;

	 stPrintHead.nInvoiceTotalPrintLineAPay = m_nInvoiceTotalPrintLineAPay;
	 stPrintHead.nInvoiceTotalPageAPay = m_nInvoiceTotalPageAPay;

	//折扣总金额
	stPrintHead.dbTotalDisAmt = InvoiceGetTotalDisAmt();

    stPrintHead.nInvoiceSaleMerchLineEnd = m_nInvoiceSaleMerchLineEnd;
	stPrintHead.nInvoiceSaleMerchLineStart = m_nInvoiceSaleMerchLineStart;
	stPrintHead.nInvoicePaymentLineEnd = m_nInvoicePaymentLineEnd;
	stPrintHead.nInvoicePaymentLineStart = m_nInvoicePaymentLineStart;
	stPrintHead.nInvoiceAmtTotalLineEnd = m_nInvoiceAmtTotalLineEnd;
	stPrintHead.nInvoiceAmtTotalLineStart = m_nInvoiceAmtTotalLineStart;

	m_InvoiceImpl.printInvoiceHead(stPrintHead,PBID_INVOICE_BNQ);
}

void CSaleImpl::InvoicePrintPayment()
{
	double  forchange_amt =0.0;
	bool bInvoicePrint = false;
	bInvoicePrint = GetSalePos()->m_params.m_bAllowInvoicePrint;
	double dbInvoiceAmt = 0.0;//实际支付金额

	const TPaymentStyle *ps = NULL;

	for ( vector<PayInfo>::const_iterator pay = m_PayImpl.m_vecPay.begin(); pay != m_PayImpl.m_vecPay.end(); ++pay) 
	{
		TPaymentItem	pPaymentItem;

		ps = m_PayImpl.GetPaymentStyle(pay->nPSID);

		if ( NULL != ps )
		{
			pPaymentItem.bPrintOnVoucher = ps->bPrintOnVoucher;	
			pPaymentItem.bForInvoiceAmt = ps->bForInvoiceAmt;
			pPaymentItem.bCallExtInterface = ps->bCallExtInterface;
		}	

		pPaymentItem.nPSID = pay->nPSID;
			
		if ( NULL != ps && ps->bNeedPayNO )
		{
			for ( vector<PayInfoItem>::iterator payInfoItem = m_PayImpl.m_vecPayItem.begin(); 
			payInfoItem != m_PayImpl.m_vecPayItem.end(); ++payInfoItem) 
			{
				if ( payInfoItem->nPSID == pay->nPSID ) 
				{
					//支付类型名称
					strcpy(pPaymentItem.szDescription,ps->szDescription);
					
					//支付金额	
					pPaymentItem.fPaymentAmt = payInfoItem->fPaymentAmt;
					
					//号码
					strcpy(pPaymentItem.szPaymentNum,payInfoItem->szPaymentNum);
					
					//若为分期付款，需要打印贷款ID、贷款期数、类型 [dandan.wu 2016-3-28]
					if ( payInfoItem->nPSID == ::GetSalePos()->GetParams()->m_nInstallmentPSID )
					{
						pPaymentItem.nLoanID = payInfoItem->nLoanID;
						pPaymentItem.nLoanPeriod = payInfoItem->nLoanPeriod;
						if ( strlen(payInfoItem->szLoanType) == 0 )
						{
							CUniTrace::Trace("销售：贷款类型为空");
						}
						else
						{
							strcpy(pPaymentItem.szLoanType,payInfoItem->szLoanType);
						}

						if ( strlen(payInfoItem->szLoanID) == 0 )
						{
							CUniTrace::Trace("销售：贷款ID为空");
						}
						else
						{
							strcpy(pPaymentItem.szLoanID,payInfoItem->szLoanID);
						}
					}					
					
					//aori add 银行卡打印隐蔽倒数第5至倒第12位
					HideBankCardNum(pay, pPaymentItem);
					
					if (pPaymentItem.fPaymentAmt >= 0 )
						GetSalePos()->GetPrintInterface()->nPrintPayment(&pPaymentItem);//打印支付							
				}
			}
		}
		else
		{
			if ( NULL != ps )
			{
				strcpy(pPaymentItem.szDescription,ps->szDescription);
			}

			//金额
			pPaymentItem.fPaymentAmt = pay->fPayAmt;
			
			if ( NULL != ps && !ps->bNeedReadCard) //非IC卡打paynum
				strcpy(pPaymentItem.szPaymentNum,pay->szPayNum);
			
			if (pPaymentItem.fPaymentAmt > 0 )
				GetSalePos()->GetPrintInterface()->nPrintPayment(&pPaymentItem);//打印支付
		}

		if ( NULL != ps && ps->bForChange)
		{
			forchange_amt -= pay->fPayAmt;
		}

	}

	//打印找零
	TPrintTotal_change pPrintTotale_change;
	double dTemp = m_PayImpl.GetChange();
	pPrintTotale_change.nPayAmt=m_PayImpl.GetPayAmt()+forchange_amt;
	pPrintTotale_change.nChange=dTemp;
	GetSalePos()->GetPrintInterface()->nPrintTotal_Change(&pPrintTotale_change);
}

void CSaleImpl::InvoiceGetTotalPageAndLine()
{
	int nAddLine = 0;  
	int nTemp = 0,nRealPageLine = 0;
	int nSize = 0 ; 
	CString strDistrict = _T("");
	vector<SaleMerchInfo>::iterator merchInfo;
	int nInvoiceNo = 0;

	int nRemain = 0;				//余数
	int nRemainLine = 0;			//一页中剩余的能打印行数
	int nMaxLineForOnePage = 0;		//一页中能打印的最大行数
	vector<CString> vecStrOrderNo;	//订单号
	vector<CString>::iterator itOrderNo;	
	const TPaymentStyle *ps = NULL;

	nMaxLineForOnePage = GetSalePos()->m_params.m_nInvoicePageMaxLine;

	//总行数和总页数先请0
	InvoiceRest();

	//获得发票号
	GetInvocieNo(m_strInvoiceNoFirst);

	strDistrict = GetSalePos()->m_params.m_strInvoiceDistrict;
	
	//头部行数统计
	nAddLine = m_InvoiceImpl.InvoiceGetHeadLine();
	m_nInvoiceHeadLine = nAddLine;
	InvoicePaging(nAddLine);

	int nStockItem = 0,nOrderItem = 0;
	for (merchInfo = m_vecSaleMerch.begin(); merchInfo != m_vecSaleMerch.end(); ++merchInfo) 
	{
		//被取消商品不算	
		if (merchInfo->nMerchState == RS_CANCEL)
		{
			continue;
		}
		
		//统计现货商品
		if ( merchInfo->nItemType == ITEM_IN_STOCK )
		{
			nStockItem++;
		}
		//获得所有订单号
		else if ( merchInfo->nItemType == ITEM_IN_ORDER && !merchInfo->strOrderNo.IsEmpty() )
		{
			nOrderItem++;
			vecStrOrderNo.push_back(merchInfo->strOrderNo);	
		}
	}

	//商品起始行 = 标题行+1
	if ( (nStockItem + nOrderItem) != 0 )
	{
		m_nInvoiceSaleMerchLineStart = m_nInvoiceTotalPrintLineSale+1;
	}

	//打印现货商品
	if ( nStockItem >= 1 )
	{
		//标题：现货商品
		InvoicePaging(1);

		//现货明细
		for (merchInfo = m_vecSaleMerch.begin(); merchInfo != m_vecSaleMerch.end(); ++merchInfo) 
		{
			//被取消商品不算	
			if (merchInfo->nMerchState == RS_CANCEL)
			{
				continue;
			}
			
			if ( merchInfo->nItemType == ITEM_IN_STOCK )
			{		
				nAddLine = InvoiceGetSaleMerchLine(merchInfo->fManualDiscount);
				InvoicePaging(nAddLine);
			}
		}
	}

	//订单号去重
	if ( vecStrOrderNo.size() > 1 )
	{
		sort(vecStrOrderNo.begin(), vecStrOrderNo.end()); 
		
		itOrderNo = unique(vecStrOrderNo.begin(),vecStrOrderNo.end());	
		vecStrOrderNo.erase( itOrderNo, vecStrOrderNo.end());
	}

	for ( itOrderNo = vecStrOrderNo.begin();itOrderNo != vecStrOrderNo.end(); ++itOrderNo )
	{
		//打印订单号
		InvoicePaging(1);
		
		//打印订单明细
		for (merchInfo = m_vecSaleMerch.begin(); merchInfo != m_vecSaleMerch.end(); ++merchInfo) 
		{
			if ( merchInfo->nMerchState != RS_CANCEL 
				&& merchInfo->nItemType == ITEM_IN_ORDER 
				&& itOrderNo->Compare(merchInfo->strOrderNo) == 0 )
			{
				//被取消商品不算	
				if (merchInfo->nMerchState == RS_CANCEL)
				{
					continue;
				}

				nAddLine = InvoiceGetSaleMerchLine(merchInfo->fManualDiscount);
				InvoicePaging(nAddLine);
			}
		}
	}
	
	//商品最后一行
	m_nInvoiceSaleMerchLineEnd = m_nInvoiceTotalPrintLineSale;

	//在打印支付之前，若该页有商品，即商品的最后一行与当前行在同一页，打印小计 [dandan.wu 2016-11-15]
	int nCurLine = m_nInvoiceTotalPrintLineSale+1;
	int nPage = m_InvoiceImpl.InvoiceGetPageFromLine(nCurLine);
	int nSaleMerchPageEnd = m_InvoiceImpl.InvoiceGetPageFromLine(m_nInvoiceSaleMerchLineEnd);	
	if ( nPage == nSaleMerchPageEnd )
	{
		InvoicePaging(INVOICE_LINE_SUBTATAL);
	}

	//支付部分
	m_nInvoicePaymentLineStart = m_nInvoiceTotalPrintLineSale+1;
	for ( vector<PayInfo>::const_iterator pay = m_PayImpl.m_vecPay.begin(); pay != m_PayImpl.m_vecPay.end(); ++pay) 
	{
		if ( pay->nPSID == GetSalePos()->m_params.m_nSavingCardPSID)
		{
			m_bHasPrepaidCardPS = true;
		}

		ps = m_PayImpl.GetPaymentStyle(pay->nPSID);

		if ( NULL != ps && ps->bPrintOnVoucher)//能在小票上打印
		{	
			if ( ps->bNeedPayNO )
			{
				for ( vector<PayInfoItem>::iterator payInfoItem = m_PayImpl.m_vecPayItem.begin(); 
					payInfoItem != m_PayImpl.m_vecPayItem.end(); ++payInfoItem) 
				{
					if ( payInfoItem->nPSID == pay->nPSID ) 
					{
						//分期付款
						if ( pay->nPSID == GetSalePos()->m_params.m_nInstallmentPSID)
						{
							nAddLine = INVOICE_LINE_PS_INSTALLMENT;
						}
						//支付宝/微信有机具时不打印卡号，机具打印
						//直连，需要打印卡号 [dandan.wu 2017-10-19]
						else if ( pay->nPSID == GetSalePos()->m_params.m_nAliPayPSID 
							||  pay->nPSID == GetSalePos()->m_params.m_nWeChatPayPSID )
						{
							if ( ps->bCallExtInterface )
							{
								nAddLine = INVOICE_LINE_PS_WITH_NO;
							}
							else
							{
								nAddLine = INVOICE_LINE_PS_WITHOUT_NO;
							}
						}
						else 
						{
							nAddLine = INVOICE_LINE_PS_WITH_NO;
						}

						//带卡号的支付方式	
						InvoicePaging(nAddLine);
					}					
				}		
			}	
			else
			{
				nAddLine = 1;

				//不带卡号的支付方式	
				InvoicePaging(nAddLine);
			}
		}	
	}
	m_nInvoicePaymentLineEnd = m_nInvoiceTotalPrintLineSale;

	//找零
	InvoicePaging(1);

	//会员卡号
	if ( strlen(GetMember().szMemberCode) > 0  )
	{
		InvoicePaging(1);
	}
	
	//交易号
	InvoicePaging(1);

	//+总计3行
	//若是青岛发票，因为格式打印，若一张发票允许最多打印20行，最后一张发票实际上打印了23行
	//if ( strDistrict != INVOICE_DISTRICT_QD )
	{	
		m_nInvoiceAmtTotalLineStart = m_nInvoiceTotalPrintLineSale+1;
		InvoicePaging(INVOICE_LINE_TOTAL);
		m_nInvoiceAmtTotalLineEnd = m_nInvoiceTotalPrintLineSale;
	}

	//若有储值卡这种支付方式，打印完毕后，切纸，再单独打印一张给收银员留存，用来对账
	InvoiceGetTotalPageAndLinePrepaid();

	//支付宝/微信直连，单独打印
	InvoiceGetTotalPageAndLineAPay();

	//必须一开始在这儿这个赋值，后续要用到
	m_nInvoiceTotalPage = m_nInvoiceTotalPageSale+m_nInvoiceTotalPagePrepaid+m_nInvoiceTotalPageAPay;
	
	//为每个商品赋值该卷发票的末尾号
	CString strInvoiceNoTemp = _T("");
	int nInvoiceNoTemp = 0; 
	for ( merchInfo = m_vecSaleMerch.begin(); merchInfo != m_vecSaleMerch.end(); ++merchInfo )
	{
		//被取消商品不算	
		if (merchInfo->nMerchState == RS_CANCEL)
		{
			continue;
		}	
		
		nInvoiceNoTemp = atoi(m_strInvoiceNoFirst) + m_nInvoiceTotalPage-1;
		strInvoiceNoTemp.Format("%08d",nInvoiceNoTemp);//发票号8位，不足补0

		//该笔交易的最后一张发票号
		merchInfo->strInvoiceEndNo = strInvoiceNoTemp;

		//更新表SaleItem_Temp的InvoiceEndNo
		m_InvoiceImpl.UpdateSaleItemInvoiceNo(strInvoiceNoTemp,merchInfo->szPLUInput);
	}
}

int CSaleImpl::InvoiceGetSaleMerchLine(double dbfManualDiscount)
{
	int nAddLine = 0;

	//商品做MarkDown，打印3行；商品做MarkUp,打印2行 [dandan.wu 2017-11-8]
	if ( dbfManualDiscount > 0 )
	{
		nAddLine = INVOICE_LINE_SALEMERCH_MARKDOWN;
	}
	else
	{
		nAddLine = INVOICE_LINE_SALEMERCH;
	}	
	
	return nAddLine;
}

void CSaleImpl::InvoicePaging(const int& nAddLine)
{
	int nRemain = 0;				//余数
	int nRemainLine = 0;			//一页中剩余的能打印行数
	int nMaxLineForOnePage = 0;		//一页中能打印的最大行数	
	int nRealNeedLine = 0;			//实际需要打印的行数，可能要包含小计
	bool bPrintSubTotal = false;	//需要打印小计
	int nBlankLine = 0;
	int nPage = 0;
	int nSaleMerchPageEnd = 0;

 	nMaxLineForOnePage = GetSalePos()->m_params.m_nInvoicePageMaxLine;
	nRemain = m_nInvoiceTotalPrintLineSale % nMaxLineForOnePage;
	nPage = m_InvoiceImpl.InvoiceGetPageFromLine(m_nInvoiceTotalPrintLineSale);
	nSaleMerchPageEnd = m_InvoiceImpl.InvoiceGetPageFromLine(m_nInvoiceSaleMerchLineEnd);
	
	//若当前行数刚好是整数倍，则换页，打印头部
	if ( m_nInvoiceTotalPrintLineSale > 1 && nRemain == 0 )
	{
		//头部信息
 		m_nInvoiceTotalPrintLineSale += m_nInvoiceHeadLine;

		//重算余数
		nRemain = m_nInvoiceTotalPrintLineSale % nMaxLineForOnePage;
	}

	//一页中剩余的能打印行数
	nRemainLine = nMaxLineForOnePage - nRemain;

	//需要打印的行数
	nRealNeedLine += nAddLine;

	//商品还没都打印完，要打印小计
	if ( m_nInvoiceSaleMerchLineEnd == 0 )
	{
		bPrintSubTotal = true;
		nRealNeedLine += INVOICE_LINE_SUBTATAL;
	}
	else
	{
		bPrintSubTotal = false;
	}

	//剩余行数>需要打印的行数，不用换页
	if ( nRemainLine > nRealNeedLine )
	{
		//打印内容
		m_nInvoiceTotalPrintLineSale += nAddLine;
	}
	//剩余行数=需要打印的行数,不用换页
	else if ( nRemainLine == nRealNeedLine )
	{	
		//打印内容
		m_nInvoiceTotalPrintLineSale += nAddLine;

		//打印小计
		if ( bPrintSubTotal )
		{
			m_nInvoiceTotalPrintLineSale += INVOICE_LINE_SUBTATAL;
		}
	}
	//剩余行数<需要打印的行数，说明需要换页了
	else
	{	
		//先打印空行
		if ( bPrintSubTotal )
		{
			nBlankLine = nRemainLine - INVOICE_LINE_SUBTATAL;
		}
		else
		{
			nBlankLine = nRemainLine;
		}
		m_nInvoiceTotalPrintLineSale += nBlankLine;

		//打印小计
		if ( bPrintSubTotal )
		{
			m_nInvoiceTotalPrintLineSale += INVOICE_LINE_SUBTATAL;
		}

		//打印发票头
 		m_nInvoiceTotalPrintLineSale += m_nInvoiceHeadLine;

		//打印内容
		m_nInvoiceTotalPrintLineSale += nAddLine;
	}	

	//计算页数
	m_nInvoiceTotalPageSale = m_InvoiceImpl.InvoiceGetPageFromLine(m_nInvoiceTotalPrintLineSale);
}

double CSaleImpl::InvoiceGetTotalDisAmt()
{
	double dbTotalDisAmt = 0.0;

	for ( vector<SaleMerchInfo>::iterator merchInfo = m_vecSaleMerch.begin(); merchInfo != m_vecSaleMerch.end(); ++merchInfo )
	{
		if ( merchInfo->nMerchState != RS_CANCEL ) 
		{
			dbTotalDisAmt += merchInfo->dCondPromDiscount;//条件促销

			//MarkUp不计入 [dandan.wu 2017-11-8]
			if ( merchInfo->fManualDiscount > 0 )
			{
				dbTotalDisAmt += merchInfo->fManualDiscount;
			}
			  //MarkDown/MarkUp
			dbTotalDisAmt += merchInfo->dMemDiscAfterProm;//会员折扣
		}
	}

	char szBuff[32] = {0};
	sprintf(szBuff,"发票折扣总金额：%.2f",dbTotalDisAmt);
	CUniTrace::Trace(szBuff);

	return dbTotalDisAmt;
}

void CSaleImpl::HandleInvoiceRemainPage(eInvoiceRemain& eType)
{
	eType = INVOICE_REMAIN_GREATER;

	//打印机不可用则退出
	if (!GetSalePos()->GetPrintInterface()->m_bValid) 
	{
		GetSalePos()->GetWriteLog()->WriteLog("END CSaleImpl::IsChangeInvoiceSegment PrintInterface::m_bValid禁止打印",4,POSLOG_LEVEL1); 
		return;
	} 
	
	if ( !GetSalePos()->m_params.m_bAllowInvoicePrint )
	{
		GetSalePos()->GetWriteLog()->WriteLog("END CSaleImpl::IsChangeInvoiceSegment 禁止发票打印",4,POSLOG_LEVEL1); 
		return;
	}

	//获得发票总页数和总行数
	InvoiceGetTotalPageAndLine();

	m_InvoiceImpl.HandleInvoiceRemainPage(m_nSegmentID,m_strInvoiceCode,m_strInvoiceNoFirst,
		m_strInvoiceEndNo,m_nInvoiceTotalPage,eType);
	m_eInvoiceRemain = eType;

	//剩余发票数不够
	if ( eType  == INVOICE_REMAIN_LESS )
	{
		if ( GetSalePos()->m_params.m_strInvoiceDistrict == INVOICE_DISTRICT_QD )
		{					
			//进纸到最后一张发票
			m_InvoiceImpl.FeedPaper(atoi(m_strInvoiceNoFirst),atoi(m_strInvoiceEndNo));

			//打印汇总信息
			InvoicePrintSummaryInfo();
		}
		else
		{
			//剩余发票全部进纸
			m_InvoiceImpl.FeedPaper(atoi(m_strInvoiceNoFirst),atoi(m_strInvoiceEndNo)+1);
		}	

		//更新旧发票卷的发票号
		InvoiceUpdateInvoiceNO(atoi(m_strInvoiceEndNo)+1);

		//提示用户换发票卷
		CString strMsg = _T("剩余发票不足以打印本次交易，请更换新的发票卷，换好发票卷后：\n（1）重新按【开票】键打印\n（2）将剩余发票拿去后台作废");
		MessageBox(NULL,strMsg, "提示", MB_OK | MB_ICONWARNING);		
		CUniTrace::Trace(strMsg);
	}
}

void CSaleImpl::InvoiceRest()
{
	//总行数和总页数先请0
	m_nInvoiceTotalPrintLineSale = 0; 
	m_nInvoiceTotalPageSale = 0;

	m_nInvoiceSaleMerchLineEnd = 0;
	m_nInvoiceSaleMerchLineStart = 0;
	m_nInvoicePaymentLineEnd = 0;
	m_nInvoicePaymentLineStart = 0;
	m_nInvoiceAmtTotalLineEnd = 0;
	m_nInvoiceAmtTotalLineStart = 0;

	m_nInvoiceHeadLine = 0;
	m_strInvoiceNoFirst = _T("");
}

void CSaleImpl::GetInvocieNo(CString & strInvoiceNO)
{
	if ( m_strInvoiceCode.IsEmpty() || m_strInvoiceEndNo.IsEmpty() || m_strInvoiceStartNo.IsEmpty() )
	{
		CUniTrace::Trace("CSaleImpl::GetInvocieNo(),参数为空，获得发票号失败！！！");
		return;
	}

	if ( !GetSalePos()->m_params.m_bAllowInvoicePrint )
	{
		CUniTrace::Trace("CSaleImpl::GetInvocieNo(),非发票打印，获得发票号失败！！！");
		return;
	}

	m_InvoiceImpl.GetNextInvocieNo(m_strInvoiceCode,m_strInvoiceEndNo,m_strInvoiceStartNo,strInvoiceNO);
}


void CSaleImpl::CreateInvoiceSegmentNoDlg(CFinDataManage *pFinDataManager,bool bExit)
{
	int nRes = 0;
	CInvoiceNOSegment dlg;
	dlg.SetExit(bExit);
	nRes = dlg.DoModal();
	if ( nRes == IDOK)
	{
		CString strInvoiceCode(_T("")),strStartNo = _T(""),strEndNo = _T(""),strTemp(_T(""));
		int nSegmentID = 0;
		dlg.GetInvoiceInfo(nSegmentID,strInvoiceCode,strStartNo,strEndNo);		
		SetInvoiceInfo(nSegmentID,strInvoiceCode,strStartNo,strEndNo);
		pFinDataManager->SetInvoiceInfo(nSegmentID,strInvoiceCode,strStartNo,strEndNo);
		
		strTemp += _T("发票代码：");
		strTemp += strInvoiceCode;
		strTemp += _T("发票起始号：");
		strTemp += strStartNo;
		strTemp += _T("发票末尾号：");
		strTemp += strEndNo;
		CUniTrace::Trace(strTemp);
		
		//重新计算发票总页数、总行数、商品对应的发票号
		//InvoiceGetTotalPageAndLine();	

		//获得发票号
		GetInvocieNo(m_strInvoiceNoFirst);
		
		//更新发票号
		//m_InvoiceImpl.UpdateInvoiceNo(m_strInvoiceEndNo,m_strInvoiceCode,m_strInvoiceStartNo,m_strInvoiceEndNo);
	}
}

void CSaleImpl::InvoicePrintPrepaidCard()
{
	if ( !GetSalePos()->m_params.m_bInvoicePrintPrepaidCardPage)
	{
		CUniTrace::Trace("CSaleImpl::InvoicePrintPrepaidCard(),!GetSalePos()->m_params.m_bInvoicePrintPrepaidCardPage");
		return;
	}

	TPaymentItem	pPaymentItem;
	vector<TPaymentItem> vecPS;
	int nPrepaidPSID = GetSalePos()->GetParams()->m_nSavingCardPSID;

	vector<PayInfoItem> vecFindPayItem;
	m_PayImpl.GetPayInfoItemVect(nPrepaidPSID, &vecFindPayItem);

	if ( vecFindPayItem.size() < 1 )
	{
		return;
	}

	const TPaymentStyle *ps = NULL;

	for ( vector<PayInfoItem>::iterator payInfoItem = vecFindPayItem.begin(); payInfoItem != vecFindPayItem.end(); ++payInfoItem) 
	{
		//支付类型名称
		ps = m_PayImpl.GetPaymentStyle(nPrepaidPSID);
		if ( NULL != ps )
		{
			strcpy(pPaymentItem.szDescription,ps->szDescription);
		}
		
		//支付金额
		pPaymentItem.fPaymentAmt = payInfoItem->fPaymentAmt;
		
		//余额
		pPaymentItem.fRemainAmt = payInfoItem->fRemainAmt;
		
		//号码
		strcpy(pPaymentItem.szPaymentNum,payInfoItem->szPaymentNum);	
		
		vecPS.push_back(pPaymentItem);
	}

	GetSalePos()->GetPrintInterface()->fnInvoicePrintIndividual_m(vecPS,PBID_INVOICE_BNQ,INVOICE_PRINTINFO_TYPE_PREPAID);
}

void CSaleImpl::InvoiceGetTotalPageAndLinePrepaid()
{
	vector<PayInfoItem> vecFindPayItem;
	int nPrepaidPSID = GetSalePos()->GetParams()->m_nSavingCardPSID;	
	m_PayImpl.GetPayInfoItemVect(nPrepaidPSID, &vecFindPayItem);	
	if ( vecFindPayItem.size() < 1 )
	{
		return;
	}

	if ( !GetSalePos()->m_params.m_bInvoicePrintPrepaidCardPage)
	{
		CUniTrace::Trace("CSaleImpl::InvoiceGetPrepaidCardPageCount(),!GetSalePos()->m_params.m_bInvoicePrintPrepaidCardPage");
		return;
	}

	m_InvoiceImpl.InvoiceGetTotalPageAndLineIndividual(vecFindPayItem,m_nInvoiceTotalPagePrepaid,m_nInvoiceTotalPrintLinePrepaid,INVOICE_LINE_PREPAID);
}

void CSaleImpl::InvoicePrintSummaryInfo()
{
	//获得汇总信息
	TInvoicePrintSummary stSummary;
	m_InvoiceImpl.LoadRemoteInvoiceSumInfo(m_nSegmentID,m_strInvoiceCode,m_strInvoiceStartNo,m_strInvoiceEndNo,stSummary);
	
	//打印头部分
	InvoicePrintHead(m_strInvoiceEndNo,1);
	
	//打印汇总信息
	GetSalePos()->GetPrintInterface()->fnInvoicePrintSummary_m(&stSummary,PBID_INVOICE_BNQ);
}

void CSaleImpl::InvoiceUpdateInvoiceNO(const int& nInvoiceNo)
{
	CString strInvoiceNoTemp = _T("");

	strInvoiceNoTemp.Format("%08d",nInvoiceNo);//发票号8位，不足补0
	m_InvoiceImpl.UpdateInvoiceNo(strInvoiceNoTemp,m_strInvoiceCode,m_strInvoiceStartNo,m_strInvoiceEndNo);
}

void CSaleImpl::SetSaleMerchInfoPreprint(vector<SaleMerchInfo>& vecInfo)
{
	m_vecSaleMerchPrePrint = vecInfo;
}
CString CSaleImpl::FormatMiyaDiscount(CString strDiscount)
{
	double dbDiscount;
	dbDiscount = atof(strDiscount);
	dbDiscount = (int)(dbDiscount + 0.5);
	strDiscount.Format("%.2f",dbDiscount/100);
	return strDiscount;
}

void CSaleImpl::SetPromoterID(CString strPromoterID)
{
	m_strPromoterID = strPromoterID;
}

void CSaleImpl::InvoiceGetTotalPageAndLineAPay()
{
	int nAliPayPSID = 0,nWeChatPayPSID = 0;
	vector<PayInfoItem> vecPayItemAliPay;
	vector<PayInfoItem> vecPayItemWeChatPay;
	const TPaymentStyle *ps = NULL;

	nAliPayPSID = GetSalePos()->GetParams()->m_nAliPayPSID;
	nWeChatPayPSID = GetSalePos()->GetParams()->m_nWeChatPayPSID;
	
	ps = m_PayImpl.GetPaymentStyle(nAliPayPSID);
	if ( ps != NULL && ps->bCallExtInterface )
	{
		m_PayImpl.GetPayInfoItemVect(nAliPayPSID, &vecPayItemAliPay);
	}

	ps = m_PayImpl.GetPaymentStyle(nWeChatPayPSID);
	if ( ps != NULL && ps->bCallExtInterface )
	{
		m_PayImpl.GetPayInfoItemVect(nWeChatPayPSID, &vecPayItemWeChatPay);
	}

	if ( vecPayItemAliPay.size() < 1 && vecPayItemWeChatPay.size() < 1 )
	{
		CUniTrace::Trace("没有支付宝和微信（直连），不需要单独打印");
		return;
	}

	for ( vector<PayInfoItem>::iterator itAliPay = vecPayItemAliPay.begin(); itAliPay != vecPayItemAliPay.end(); ++itAliPay )
	{
		ps = m_PayImpl.GetPaymentStyle(itAliPay->nPSID);
		if ( ps != NULL )
		{
			strcpy(itAliPay->szDescription,ps->szDescription);
		}
		m_vecPayItemAPay.push_back(*itAliPay);
	}

	for ( vector<PayInfoItem>::iterator itWeChatPay = vecPayItemWeChatPay.begin(); itWeChatPay != vecPayItemWeChatPay.end(); ++itWeChatPay )
	{
		ps = m_PayImpl.GetPaymentStyle(itWeChatPay->nPSID);
		if ( ps != NULL )
		{
			strcpy(itWeChatPay->szDescription,ps->szDescription);
		}
		m_vecPayItemAPay.push_back(*itWeChatPay);
	}
	
	m_InvoiceImpl.InvoiceGetTotalPageAndLineIndividual(m_vecPayItemAPay,m_nInvoiceTotalPageAPay,m_nInvoiceTotalPrintLineAPay,INVOICE_LINE_APAY);
}