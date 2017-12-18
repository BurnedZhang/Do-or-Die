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
CSaleImpl::CSaleImpl() : m_nSaleID(1),m_stratege_xiangou(this)//,m_EndSaleMerchInfo(CString("inputplu"))2013-2-20 17:33:11 �˳����۱���
{	m_bReprint=false;
	m_IsLoadedUncompletedSale = false;
	m_strategy_WholeBill_Stamp=NULL;
	Clear();
	for (int i = 0; i<10; i++)
	{
		StyleID[i] = 0;
	}

	m_dbSumMemDis = 0.0f; //��Ա�ۿ��ܽ��
	m_dbSumPromDis = 0.0f;//������ܽ��

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

//�����������
//##ModelId=3E641FAC00F7
void CSaleImpl::Clear()
{
	//EndMerchClear(); //��������Ʒ��¼
	if(m_strategy_WholeBill_Stamp){
		delete m_strategy_WholeBill_Stamp;
		m_strategy_WholeBill_Stamp=NULL;
	}//���� 2012-9-24 17:29:34 proj 2012-9-10 R2POS T1987 �ܵ�ӡ�� 1.����ӡ�����Χ=2����һ�ʽ��׻�Ա����5��ӡ�����ڶ��ʽ��׷ǻ�Ա��Ҳ����5��ӡ��
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
	//m_bMember = false;/2013-3-4 15:05:20 proj2013-3-4-1 ��Ա�Ż�
	GetSalePos()->GetPromotionInterface()->m_vreloadPromotion.clear();//aori add ���� 2012-8-14 15:40:37 proj 2012-8-14-1 ������Ϣ����
	//��ʼ���̱���
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
//��������ȡ�ϼ���Ϣ
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
   //��ȥ�����ۿ۽��
    dTotal-=GetSalePos()->GetPromotionInterface()->m_fDisCountAmt;
	ReducePrecision(dTotal);
	
/*	//�����ݿ��

	double dTotal_db = GetSaleTotAmtFromDB();
	strLog.Format("��SaleItem_tempȡ�ý��:%f",dTotal_db);
	CUniTrace::Trace(strLog);
	strLog.Format("Fun End->CSaleImpl::GetTotalAmt->����ֵ:%f",dTotal);
	CUniTrace::Trace(strLog);*/
	return dTotal;
}

//����ϼƽ�ָ����Ʒ��ϸ����(���ۼ������������)
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
 
   //��ȥ�����ۿ۽��
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
   //��ȥ�����ۿ۽��
    dTotal-=GetSalePos()->GetPromotionInterface()->m_fDisCountAmt;//SumPromotionAmt(m_nSaleID);//aori replace m_fDisCountAmt to sum 2012-2-9 13:49:53 proj 2.79 ���Է������ش�ӡСƱ Ӧ��������;//aori del 2012-2-7 16:11:04 proj 2.79 ���Է�������������ʱ ������ �� fSaleAmt_BeforePromotion
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


//ȡ��ָ����ŵ�����
//##ModelId=3E641FAB03E4
bool CSaleImpl::CancelItem(int nSID)
{
	if ( nSID < m_vecSaleMerch.size() ) {
		m_vecSaleMerch[nSID].nMerchState = RS_CANCEL;
		//zenghl 20090608 WriteSaleItemLog
		WriteSaleItemLog(m_vecSaleMerch[nSID],1,0);	//aori add 2011-5-13 10:53:09 proj2011-5-11-2�����۲�ƽ5-9���ƴ�3: ��־���� writelogд��dll
		//add lxg.03/3/26�޸���ʱ��Ʒ��
		UpdateItemInTemp(nSID);

	}
	return true;
}

//ȡ��ָ����ŵ�����
//##ModelId=3E641FAB03E4
bool CSaleImpl::CancelItem(int nSID,char* authorizePerson)
{
	if ( nSID < m_vecSaleMerch.size()&&m_vecSaleMerch[nSID].nMerchState!=RS_CANCEL ) {//aori add ���Է�����2012-2-24 14:47:19  proj 2012-2-13 �ۿ۱�ǩУ�� ��ȫȡ��ʱ�����һ����
		SaleMerchInfo* pCancelMerch=&m_vecSaleMerch[nSID];//aori add for debuginfo
		m_vecSaleMerch[nSID].nMerchState = RS_CANCEL;
		m_vecSaleMerch[nSID].szAuthorizePerson=authorizePerson;
		//zenghl 20090608 WriteSaleItemLog
		WriteSaleItemLog(m_vecSaleMerch[nSID],1,0);	//aori add 2011-5-13 10:53:09 proj2011-5-11-2�����۲�ƽ5-9���ƴ�3: ��־���� writelogд��dll
		m_stratege_xiangou.UpdatelimitInfo(m_vecSaleMerch[nSID]);
		//add lxg.03/3/26�޸���ʱ��Ʒ��
		UpdateItemInTemp(nSID);
		//if (m_vecSaleMerch[nSID].LimitStyle<LIMITSTYLE_UPLIMIT&&m_vecSaleMerch[nSID].LimitStyle>LIMITSTYLE_NoLimit){//aori add2011-4-11 11:36:20 bug: ����Ա�ź�ɾ��Ʒ���ͬһ��Ʒ,�޹����� :
			//ReCalculateMemPrice(false);////aori add bNeedZeroLimitInfo 2011-4-8 14:33:28 limit bug ȡ����Ʒ�� �޹�����
		ReCalculateMemPrice(true);//aori change 2011-4-11 11:36:20 bug: ����Ա�ź�ɾ��Ʒ���ͬһ��Ʒ,�޹�����

		CSaleItem hh(m_vecSaleMerch[nSID].szPLUInput);
		if(hh.m_bIsDiscountLabelBar)
			GetSalePos()->GetPOSClientImpl()->checkDiscountLabel((char*)(LPCSTR)(m_vecSaleMerch[nSID].szPLUInput),false);//aori add 2012-2-16 14:45:33 proj 2012-2-13 �ۿ۱�ǩУ�� ����Ϣ ETI_DOWNLOAD_CHECK_DiscountLabel

	}
	return true;
}

void CSaleImpl::CancelOrderItem(int nSID,char* authorizePerson)
{
	if ( nSID < m_vecSaleMerch.size()&&m_vecSaleMerch[nSID].nMerchState!=RS_CANCEL )
	{
		CString strOrderNo = m_vecSaleMerch[nSID].strOrderNo;//�õ�������
		CString strPrompt;
		strPrompt.Format("�Ƿ�Ҫɾ������%s�е�������Ʒ��", strOrderNo);
		if(AfxMessageBox(strPrompt, MB_ICONQUESTION | MB_OKCANCEL) == IDOK)
		{
			for (int n = 0 ; n < (int)m_vecSaleMerch.size(); n++) 
			{
				if(m_vecSaleMerch[n].nItemType == 2 && m_vecSaleMerch[n].strOrderNo == strOrderNo)
					CancelItem(n, authorizePerson);	
			}
			CFinDataManage finDataManage;
			finDataManage.UpdateOrderStatue(strOrderNo,ORDER_UNCHARGE);//���ó�δ�շ�
		}
	}
}

//����ǰ����
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
//	����������Ϣ�������ط������õ���һ�����������ۣ����ǹ�������
//	ͬʱ���������ʱ���е�����
//
//======================================================================
// �� �� ����Save
// ��������������������Ϣ
// ���������CPosClientDB* pDB, int nSaleID, unsigned char nSaleStatus
// ���������bool
// �������ڣ�00-2-21
// �޸����ڣ�2009-4-1 ������
//�޸�����20090526 �޸�Ϊÿ�α�������ʱ����LoadSaleInfo(
// ��  �ߣ�***
// ����˵�������ݿ����޸� SaleItem ,Sale
//======================================================================
bool CSaleImpl::Save(CPosClientDB* pDB, int nSaleID, unsigned char nSaleStatus)
{	
	GetSalePos()->GetPromotionInterface()->SetSaleImpl(this);//aori move from down 2011-9-6 17:57:52 �Ż�CSaleImpl::Save
	double fTotDiscount = 0;//GetSalePos()->GetPromotionInterface()->m_fDisCountAmt;
	double fTotAmt = 0.0f, dTemp = 0.0f, fTempMemdiscount = 0.0f;
	double dManualDiscount = 0;
	CString strSQL, strManualDiscount, strSaleDT = ::GetFormatDTString(m_stEndSaleDT);//�޸�24Сʱ���۵����⣬��saledt�޸�Ϊ��Ʊ��ʱ��//	CString strSaleDT = ::GetFormatDTString(m_stSaleDT);
	const int nUserID = GetSalePos()->GetCasher()->GetUserID();
	const char *szTableDTExt = GetSalePos()->GetTableDTExt(&m_stEndSaleDT);
	try	{//fTotDiscount=GetSalePos()->GetPromotionInterface()->m_fDisCountAmt;GetSalePos()->GetPromotionInterface()->SetSaleImpl(this);
		
		CUniTrace::Trace("!!!���ձ���ǰһ�� ��������");//aori add 2011-8-3 17:03:07 proj2011-8-3-1 ���۲�ƽ �������� ��ʱ�� ����־�� //GetSalePos()->GetPromotionInterface()->WritePromotionLog( nSaleID);//aori add 2011-8-3 17:03:07 proj2011-8-3-1 ���۲�ƽ �������� ��ʱ�� ����־�� 
		
		GetSalePos()->GetPromotionInterface()->LoadSaleInfo(nSaleID,true);//20090526 �޸�Ϊÿ�α�������ʱ����LoadSaleInfo(//����������
	  	
		for ( vector<SaleMerchInfo>::iterator merchInfo = m_vecSaleMerch.begin(); merchInfo != m_vecSaleMerch.end(); ++merchInfo ) {
			if ( merchInfo->nMerchState != RS_CANCEL ) {//fTotDiscount += merchInfo->fSaleDiscount;//�Ժ�tot���治�ӻ�Ա���� + merchInfo->fMemSaleDiscount;
				fTotAmt += merchInfo->fSaleAmt;//���浽��������//���������ۿ��ܼ� //ReSortSaleMerch(); zenghl 20090528
				//fTempMemdiscount += merchInfo->fMemSaleDiscount;
				fTotDiscount += merchInfo->dCondPromDiscount;//��������
				dManualDiscount += merchInfo->fManualDiscount;
				fTempMemdiscount += merchInfo->dMemDiscAfterProm;//��Ա
			}
		}
		m_strategy_WholeBill_Stamp->GeneralYinHuaInfo_forBill();//aori add ���� 2012-9-11 17:06:26 proj 2012-9-10 R2POS T1987 �ܵ�ӡ��

		string strCC;
		if ( m_strCustomerCharacter.size() > 0 ) {
			strCC = "'";strCC += m_strCustomerCharacter;strCC += "'";
		} else
			strCC = "NULL";
		CTime tmSaleDT(m_stSaleDT), tmEndSaleDT(m_stEndSaleDT), tmCheckSaleDT(m_stCheckSaleDT);
		int nEndSaleSpan = (tmEndSaleDT - tmSaleDT).GetTotalSeconds();
		if (nEndSaleSpan < 0 )//С��0���ݴ���
			nEndSaleSpan = 15;
		int nCheckSaleSpan = (tmCheckSaleDT - tmSaleDT).GetTotalSeconds();
		if (nCheckSaleSpan < 0 )//С��0���ݴ���
			nCheckSaleSpan = 10;

		ReducePrecision(fTotDiscount);//�ۿ�ȡ������
		//��Աinput
		//cardtype=0;
		//MemberCode = "";
		//Get_Member_input();
		//��������
		//2015-3-6���ӵ���֧���ֶ�AddCol6
		//2015-3-23���ӵ���״̬�ֶ�Addcol8
		//�ٰ��Ӱ汾 addcol6�ֶδ�ɻ�Ա id=��Ա����
		CString strAddCol6 = "";
		CString strMemCode = "";
		strMemCode.Format("%s", m_MemberImpl.m_Member.szInputNO);
		if(!strMemCode.IsEmpty())
		{
			strAddCol6.Format("%d=%s", m_MemberImpl.m_Member.nHumanID, m_MemberImpl.m_Member.szMemberName);
			//30λ�ض�
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
			          " MemberCardType,Ecardno,userID,MemberCardLevel,MemberCardchannel,PromoterID  ) "//���� 2012-9-14 16:07:35 proj 2012-9-10 R2POS T1987 �ܵ�ӡ�� ����
				"VALUES ('%s',%d,%d,%ld,%ld,%.4f,%.4f,%d,%d,'%s',%s,%.4f,%.4f,0,%f,%d,%d,%d ,'%s',%d,'%s','%s','%d' ,'%s','%s','%s','%s','%s','%s')", 
				szTableDTExt, strSaleDT, nEndSaleSpan, nCheckSaleSpan, 
				nSaleID, nUserID, fTotAmt, fTotDiscount, 
				nSaleStatus, GetSalePos()->GetCurrentTimePeriod(),  m_MemberImpl.m_Member.szMemberCode,//GetFieldStringOrNULL(m_MemberImpl.m_Member.szMemberCode), 
				strCC.c_str(), /*m_fDiscount*/dManualDiscount, fTempMemdiscount,m_MemberImpl.m_dMemIntegral,m_uShiftNo,
						/*m_strategy_WholeBill_Stamp->m_YinHuaCount*/0,//�ٰ���û�����ҵ������ֶ�Ӧ����0���������[dandan.wu 2016-3-10]
						m_strategy_WholeBill_Stamp->m_StampGivingMinAmt,
						m_MemberImpl.m_Member.szInputNO,
						m_MemberImpl.m_Member.nCardNoType,strAddCol6/*m_EorderID*/,m_EMemberID,m_EMOrderStatus,
						m_MemberImpl.m_Member.szCardTypeCode,m_MemberImpl.m_Member.szEcardno,m_MemberImpl.m_Member.szUserID,
						m_MemberImpl.m_Member.szMemberCardLevel,m_MemberImpl.m_Member.szMemberCardchannel,m_strPromoterID);//�ѻ�Ա����������Ҳ�㵽����ȥm_fMemDiscount); //��Աƽ̨
		if ( pDB->Execute2(strSQL) == 0){//aori change 2011-3-22 16:51:36 �Ż�clientdb ��ɾ��execute2
			CString hh;hh.Format("!!!checkoutDLG::oncheck���浽sale%sʱʧ��",szTableDTExt);CUniTrace::Trace(hh);return false;
		}
		//����������ϸ
		int nItemID = 0;
		bool bHasCanceledItems = false;
		double fSaleQuantity = 0.0f,fAddDiscount = GetItemDiscount();
		for ( merchInfo = m_vecSaleMerch.begin(); merchInfo != m_vecSaleMerch.end(); ++merchInfo ) {
			if ( merchInfo->nMerchState == RS_CANCEL ) {//ȡ����Ʒ�����ڲ�����¼Remark��, FunID = 1 --- ȡ������Ʒ
				Sleep(5);//�ӳ�1����ִ�У���ֹʱ�������ظ�
				m_pADB->SetOperateRecord(OPT_CANCELITEM, nUserID, merchInfo->szPLU);
				merchInfo->nSID=merchInfo-m_vecSaleMerch.begin();merchInfo->SetCancelSaleItem(m_stEndSaleDT, nSaleID, nUserID,1,(char*)(LPCSTR)merchInfo->szAuthorizePerson );
				bHasCanceledItems = true;
			} else {
				nItemID++;
				double fActualPrice = 0.0f;
				merchInfo->OutPut_Quantity_Price(fSaleQuantity, fActualPrice);
				if ( merchInfo->nMerchState == RS_DS_MANUAL ) {//������ֶ����ۣ���ӡ���ֶ��������
					if ( merchInfo->nLowLimit == SDS_RATE ) {
// 						if(merchInfo->bFixedPriceAndQuantity)
// 						{
// 							dTemp = (merchInfo->fActualPrice * merchInfo->fSaleQuantity) * (100.0f - merchInfo->fZKPrice) / 100.0f;
// 						}
// 						else	
// 						{
// 							dTemp = (merchInfo->fActualPrice * merchInfo->nSaleCount) * (100.0f - merchInfo->fZKPrice) / 100.0f;
// 						}
						//�������������fSaleQuantity�� [dandan.wu 2016-4-26]
						dTemp = (merchInfo->fActualPrice * merchInfo->fSaleQuantity) * (100.0f - merchInfo->fZKPrice) / 100.0f;
						ReducePrecision(dTemp);
					} else {
						dTemp = merchInfo->fZKPrice;
					}
					strManualDiscount.Format("%.4f", dTemp);
				} else {
					strManualDiscount = "NULL";
				}//�����Ʒ���ڴ���������û�е����������Ʒ�Χ�ڣ�����������Ʒ������־
				if(merchInfo->nMerchState == RS_NORMAL && merchInfo->nDiscountStyle != DS_NO)
				{
					merchInfo->nDiscountStyle = DS_NO;//merchInfo->nMakeOrgNO = 0;//merchInfo->nSalePromoID = 0;
				}
				double fTempSaleDiscount = merchInfo->fSaleDiscount;//�ۿ�ȡ������
				if(fabs(ReducePrecision(fTempSaleDiscount)) > 0)
				{
					fTempSaleDiscount += fAddDiscount;
					fAddDiscount = 0.0f;
				}
				if(merchInfo->bIsDiscountLabel )
					merchInfo->fBackDiscount = 	(merchInfo->fSysPrice - merchInfo->fActualPrice) * merchInfo->fSaleQuantity/*nSaleCount*/;
				else 
					merchInfo->fBackDiscount = 	(merchInfo->fSysPrice - fActualPrice) * merchInfo->fSaleQuantity/*nSaleCount*/;
				//����IsDiscountLabel, BackDiscount�ֶ�
				CString str_fSaleAmt_BeforePromotion;str_fSaleAmt_BeforePromotion.Format("%.2f",merchInfo->fSaleAmt_BeforePromotion);//aori add 2012-2-10 10:18:18 proj 2.79 ���Է�������������ʱ Ӧ������δ�ܱ��桢�ָ���Ʒ�Ĵ���ǰsaleAmt��Ϣ��db�����ֶα������Ϣ
//<--#������ýӿ�#				
//				strSQL.Format("INSERT INTO SaleItem%s(SaleDT,ItemID,SaleID,"
//					"PLU,RetailPrice,SaleCount,SaleQuantity,SaleAmt,"
//					"PromoStyle,MakeOrgNO,PromoID,ManualDiscount, SalesHumanID, MemberDiscount,NormalPrice,ScanPLU,StampCount"//aori add StampCount 2011-8-30 10:13:21 proj2011-8-30-1 ӡ������
//					",AddCol1 "//aori add 2012-2-10 10:18:18 proj 2.79 ���Է�������������ʱ Ӧ������δ�ܱ��桢�ָ���Ʒ�Ĵ���ǰsaleAmt��Ϣ��db�����ֶα������Ϣ
//					") VALUES ('%s',%d,%ld,'%s',%.4f,%ld,%.4f,%.4f,%d,%d,%d,%.4f,%d,%.4f,%.2f,'%s',%d"//aori add 2011-8-30 10:13:21 proj2011-8-30-1 ӡ������
//					",'%s'"//aori add 2012-2-10 10:18:18 proj 2.79 ���Է�������������ʱ Ӧ������δ�ܱ��桢�ָ���Ʒ�Ĵ���ǰsaleAmt��Ϣ��db�����ֶα������Ϣ
//					")",
//					szTableDTExt, strSaleDT, nItemID, nSaleID, 
//					merchInfo->szPLU, fActualPrice, merchInfo->nSaleCount, 
//					fSaleQuantity, merchInfo->fSaleAmt,
//					merchInfo->nPromoStyle, 
//					merchInfo->nMakeOrgNO, merchInfo->nPromoID,
//					merchInfo->fItemDiscount,merchInfo->nSimplePromoID,merchInfo->fMemberShipPoint,merchInfo->fSysPrice,merchInfo->szPLUInput,merchInfo->StampCount////aori add 2011-8-30 10:13:21 proj2011-8-30-1 ӡ������
//					,str_fSaleAmt_BeforePromotion //aori add 2012-2-10 10:18:18 proj 2.79 ���Է�������������ʱ Ӧ������δ�ܱ��桢�ָ���Ʒ�Ĵ���ǰsaleAmt��Ϣ��db�����ֶα������Ϣ
//					);
				if(m_ServiceImpl.HasSVMerch() )
					strSQL.Format("INSERT INTO SaleItem%s(SaleDT,ItemID,SaleID,"
						"PLU,RetailPrice,SaleCount,SaleQuantity,SaleAmt,"
						"PromoStyle,MakeOrgNO,PromoID,ManualDiscount, SalesHumanID, MemberDiscount,NormalPrice,ScanPLU,StampCount" 
						",AddCol1,AddCol2,AddCol3, AddCol4,ItemType,OrderID,CondPromDiscount,ArtID,OrderItemID,MemDiscAfterProm" 
						") VALUES ('%s',%d,%ld,'%s',%.4f,%ld,%.4f,%.4f,%d,%d,%d,%.4f,%d,%.4f,%.2f,'%s',%d" //%ld to %.4f 2013-2-27 15:30:11 proj2013-1-23-1 ������Ʒ ���� ��Ա�������޹�
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
					//����ӡ��Ʊ������Ʒ�ķ�Ʊ��Ϣ���浽�±���[dandan.wu 2016-10-14]
					double dbInvoiceAmt = 0.0;

					if ( GetSalePos()->m_params.m_bAllowInvoicePrint )
					{
						//��Ʊ����Ϊ������Ʒ��Ϊ0 [dandan.wu 2016-11-10]		
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
						") VALUES ('%s',%d,%ld,'%s',%.4f,%ld,%.4f,%.4f,%d,%d,%d,%.4f,%d,%.4f,%.2f,'%s',%d" //%ld to %.4f2013-2-27 15:30:11 proj2013-1-23-1 ������Ʒ ���� ��Ա�������޹�
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
				if ( retn== 0 ){//aori change 2011-3-22 16:51:36 �Ż�clientdb ��ɾ��execute2
					unlock_saleitem_temp();//aori add 2011-8-7 8:38:11	proj2011-8-3-1 A �� mutex���ж�����checkout֮�� �Ƿ������update�¼�
					return false;
				}
			}
			//����в�����־�����������
			if ( bHasCanceledItems ) {
				GetSalePos()->m_Task.putOperateRecord(m_stEndSaleDT, true);
			}
		}
		//CUniTrace::Trace("!!!���ձ���� ��������---------����־ �� 1�����洢���� ��־֮�� ��Ӧ�ó��ֶ� saleitem_temp����и��ĵ���־");//aori add 2011-8-3 17:03:07 proj2011-8-3-1 ���۲�ƽ �������� ��ʱ�� ����־�� //GetSalePos()->GetPromotionInterface()->WritePromotionLog( nSaleID);//aori add 2011-8-3 17:03:07 proj2011-8-3-1 ���۲�ƽ �������� ��ʱ�� ����־�� 

		
		strSQL.Format("DELETE FROM Sale_Temp WHERE HumanID = %d;", nUserID);//�����ʱ����
		pDB->Execute2(strSQL);//aori 2011-8-7 8:38:11  proj2011-8-3-1 �� mutex���ж�����cchange 2011-3-22 16:51:36 �Ż�clientdb ��ɾ��execute2
		strSQL.Format("DELETE FROM SaleItem_Temp WHERE HumanID = %d", nUserID);
		pDB->Execute2(strSQL);//aori 2011-8-7 8:38:11  proj2011-8-3-1 �� mutex���ж�����cchange 2011-3-22 16:51:36 �Ż�clientdb ��ɾ��execute2
		
		//EmptyTempSale();
		unlock_saleitem_temp();//aori add 2011-8-7 8:38:11	proj2011-8-3-1 A �� mutex���ж�����checkout֮�� �Ƿ������update�¼�
	} catch ( CException *pe ) {
		GetSalePos()->GetWriteLog()->WriteLogError(*pe);
		pe->Delete( );
		unlock_saleitem_temp();//aori add 2011-8-7 8:38:11	proj2011-8-3-1 A �� mutex���ж�����checkout֮�� �Ƿ������update�¼�
		return false;
	}
	return true;
}

//
//���ĳ�����۵���ǰ������
//
//##ModelId=3E641FAB02FE
bool CSaleImpl::UnHangupSale(int nSaleID, SYSTEMTIME* stTime)
{
	SYSTEMTIME stNow;(stTime == NULL)?GetLocalTimeWithSmallTime(&stNow):stNow = *stTime;
	Clear();
	
	SYSTEMTIME stUnHunupDT;GetLocalTimeWithSmallTime(&stUnHunupDT);//aori move from down 2011-6-10 11:09:25 proj2011-6-10-1:���ʹ���db����
	if(!IsInSameDay(stUnHunupDT, m_stSaleDT)){//���24Сʱ���⣬���������Ҫ���»��ʱ��
		m_nSaleID = m_pADB->GetNextSaleID(&stUnHunupDT);
	}
	m_stSaleDT = stUnHunupDT;
	m_nSaleID=nSaleID;//aori add 2011-6-10 11:09:25 proj2011-6-10-1:���ʹ���db����

	//((CPosApp *)AfxGetApp())->GetSaleDlg()->InitNextSale(SaleFinish_UnHangupSale,nSaleID);//aori add 2011-6-10 11:09:25 proj2011-6-10-1:���ʹ���db����
	//InitialNextSale(SaleFinish_UnHangupSale,nSaleID);//aori change 2011-6-10 11:09:25 proj2011-6-10-1:���ʹ���db����
	

	if ( !ReloadSale(nSaleID, stNow,true) ) 
		return false;
	WriteSaleLog(1);
	CString strSpanBegin, strSpanEnd, strSql;
	const char *szTableDTExt = GetSalePos()->GetTableDTExt(&stNow);

	strSpanBegin = GetDaySpanString(&stNow);
	strSpanEnd = GetDaySpanString(&stNow, false);
		//ɾ������������Ϣ
		strSql.Format("DELETE FROM Sale%s WHERE SaleID = %lu "
			" AND SaleDT >= '%s' AND SaleDT < '%s' ",
			szTableDTExt, nSaleID, strSpanBegin, strSpanEnd);
	CAutoDB db(true);db.Execute(strSql);//aori change 2011-3-22 16:51:36 �Ż�clientdb ��ɾ��execute2

		//ɾ��������ϸ��Ϣ
		strSql.Format("DELETE FROM SaleItem%s WHERE SaleID = %lu" 
			" AND SaleDT >= '%s' AND SaleDT < '%s' ",
			szTableDTExt, nSaleID, strSpanBegin, strSpanEnd);
		db.Execute(strSql);//aori change 2011-3-22 16:51:36 �Ż�clientdb ��ɾ��execute2

		strSql.Format("%d", nSaleID);
		m_pADB->SetOperateRecord(OPT_UNHANGUP, GetSalePos()->GetCasher()->GetUserID(), strSql);
		GetSalePos()->m_Task.putOperateRecord(stNow);

	//����ǰ��¼��¼����ʱ����
	SaveItemsToTemp();
	CString strSQL;strSQL.Format("INSERT INTO Sale_Temp(SaleDT,HumanID,MemberCode,Shift_No) VALUES"
		"('%s',%d,%s,%d)", GetFormatDTString(m_stSaleDT), 
		GetSalePos()->GetCasher()->GetUserID(),GetFieldStringOrNULL(m_MemberImpl.m_Member.szMemberCode),m_uShiftNo);
	db.Execute(strSQL);//aori change 2011-3-22 16:51:36 �Ż�clientdb ��ɾ��execute2

	return true;
}

//�������ۣ�����Ϊ���ý��п������ý��Ϊ�����������ã�
//�����ۿ���
//##ModelId=3E641FAC0061
void CSaleImpl::WholeBillRebate(double fRebateAmt, double fMemRebateAmt)
{	
	double fRest = 0.0, fMemRest = 0.0;
	
	//��������Ա����������
	if(fMemRebateAmt != 0.0 )
	{
		double fMemAmt = GetTotalAmt(true);
		//�ۿ��ʱ���0.98	
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
	//֮�����ֶ���������֮ǰ��
	if(fRebateAmt != 0.0 )
	{
		double fAmt = GetTotalAmt();
		double fRebateRate = 1.0 - (fRebateAmt / fAmt); 
		for ( vector<SaleMerchInfo>::iterator merchInfo = m_vecSaleMerch.begin(); merchInfo != m_vecSaleMerch.end(); ++merchInfo ) {
			if ( (merchInfo->nMerchState != RS_CANCEL) && ( merchInfo->fSaleQuantity/*nSaleCount*/ != 0 ) && (!merchInfo->bIsDiscountLabel)) {			
				fAmt = merchInfo->fSaleAmt;
				merchInfo->fSaleAmt *= fRebateRate;
				fRest += ReducePrecision(merchInfo->fSaleAmt);merchInfo->fSaleAmt_BeforePromotion=merchInfo->fSaleAmt;//aori add 2012-2-7 16:11:04 proj 2.79 ���Է�������������ʱ ������ �� fSaleAmt_BeforePromotion
				merchInfo->fSaleDiscount += fAmt - merchInfo->fSaleAmt;
			}
		}
	}

	// fRest�Ǹ������۵����۽��Ĩ��С�������λ�������ۼ�
	// ������̯���۸���������۶����fRest����������ȥ
	//ע�⣬��Ա�ĺͷǻ�Ա��Ҫ�ֿ�
	EraseMemOddment(fMemRest);
	EraseOddment(fRest);
}
//
//��Ա���۶�Ĩ��  
//
void CSaleImpl::EraseMemOddment(double fMemReducePrice)
{
	int nSID = 0;
	double fMaxPrice = 0.0f;
	vector<SaleMerchInfo>::iterator merchInfo = NULL;

	// �ҳ����۶����Ĩ���Ҽ۸�������Ʒ���������Ĩ��
	for ( merchInfo = m_vecSaleMerch.begin(); merchInfo != m_vecSaleMerch.end(); ++merchInfo ) {
		if( merchInfo->fSaleAmt > fMemReducePrice && merchInfo->fSaleQuantity/*nSaleCount*/ != 0 && merchInfo->fMemSaleDiscount > 0) {
			if ( merchInfo->fActualPrice > fMaxPrice ) {
				fMaxPrice = merchInfo->fActualPrice;
				nSID = merchInfo-m_vecSaleMerch.begin();
			}
		}
	}

//	��������ڣ���ȡ��һ��(��nSID=0��һֱû�б仯

	// �޸���Ʒ�����۶�ۿ۶��Լ�ʵ���ۼ�
	//fMaxPrice will be a temp double var here
	merchInfo = &m_vecSaleMerch[nSID];
	fMaxPrice = merchInfo->fSaleAmt;
	merchInfo->fSaleAmt += fMemReducePrice;
	merchInfo->fMemSaleDiscount += ReducePrecision(merchInfo->fSaleAmt);merchInfo->fSaleAmt_BeforePromotion=merchInfo->fSaleAmt;//aori add 2012-2-7 16:11:04 proj 2.79 ���Է�������������ʱ ������ �� fSaleAmt_BeforePromotion
	merchInfo->fMemSaleDiscount += fMaxPrice - merchInfo->fSaleAmt;
}

//
//���۶�Ĩ��  
//�Ҹģ������ĳ��ҳ����۶����Ĩ���Ҽ۸�������Ʒ���������Ĩ��
//
//##ModelId=3E641FAB02C2
void CSaleImpl::EraseOddment(double fReducePrice)
{
	int nSID = 0;
	double fMaxPrice = 0.0f;
	vector<SaleMerchInfo>::iterator merchInfo = NULL;

	// �ҳ����۶����Ĩ���Ҽ۸�������Ʒ���������Ĩ��
	for ( merchInfo = m_vecSaleMerch.begin(); merchInfo != m_vecSaleMerch.end(); ++merchInfo ) {
		if( merchInfo->fSaleAmt > fReducePrice && merchInfo->fSaleQuantity/*nSaleCount*/ != 0 ) {
			if ( merchInfo->fActualPrice > fMaxPrice ) {
				fMaxPrice = merchInfo->fActualPrice;
				nSID = merchInfo-m_vecSaleMerch.begin();
			}
		}
	}

//	��������ڣ���ȡ��һ��(��nSID=0��һֱû�б仯

	// �޸���Ʒ�����۶�ۿ۶��Լ�ʵ���ۼ�
	//fMaxPrice will be a temp double var here
	merchInfo = &m_vecSaleMerch[nSID];
	fMaxPrice = merchInfo->fSaleAmt;
	merchInfo->fSaleAmt += fReducePrice;
	merchInfo->fSaleDiscount += ReducePrecision(merchInfo->fSaleAmt);merchInfo->fSaleAmt_BeforePromotion=merchInfo->fSaleAmt;//aori add 2012-2-7 16:11:04 proj 2.79 ���Է�������������ʱ ������ �� fSaleAmt_BeforePromotion
	merchInfo->fSaleDiscount += fMaxPrice - merchInfo->fSaleAmt;
}
// bool CSaleImpl::checkDiscountLabelBar(){//aori add 2012-2-14 14:34:07 proj 2012-2-13 �ۿ۱�ǩУ��
// 	
// 	int nCount = -1;ETICommand ecmd;//aori add bug 2011-1-12 8:43:33 �ڴ�й¶
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
//	������Ʒ��ϸ
//
//======================================================================��
// �� �� ����AddItem
// ����������������Ʒ��ϸ
// ���������
// ���������bool
// �������ڣ�00-2-21
// �޸����ڣ�2009-4-2 ������
// ��  �ߣ�***
// ����˵������������޸�,��Ҫ��������
//======================================================================/

int CSaleImpl::AddItem(CString szPLU, Inputer inputer,bool bStamp,int scantime,
					   CString strOrderNo, CString strOrderOtemID, double dCondPromDis,
					   double dSaleQty, double dSaleAmt, int nDiscType, double dOrderSalePrice)
{//�����жϸ���Ʒ�Ƿ��Ѿ����ڣ��Ѿ����ڵĻ����󲿷���Ϣ����Ҫ�����ݿ��ж�ȡ��
//����������������ٶ�,���ǽ���͵Ļ�������ȡ����Ȼ����ƴ���������һ��m_bCanChangePrice��־�Ͳ�����
	SaleMerchInfo merchInfo("");
	SaleMerchInfo* existMerchInfo = inputer == Inputer_UpLimit_simulate ? NULL : GetSaleMerchInfo(szPLU, bStamp ? DS_YH : DS_NO);
	// bool bPriceInBarcode=false;//д��13λ����־ //aori del
// 	if ( existMerchInfo && existMerchInfo->nManagementStyle == MS_DP) {//aori change bug:
// 		merchInfo=*existMerchInfo;//2013-2-4 17:15:49 proj2013-1-17-3 �Ż� saleitem ������ͬ��Ʒ����memcpy(&merchInfo, (void *)existMerchInfo, sizeof(SaleMerchInfo));
// 		merchInfo.bLimitSale=FALSE;//aori add for bug:nonmember get memberprice
// 		merchInfo.bIsDiscountLabel=FALSE;//aori add 2011-1-4 9:41:08 ����ͬһ��Ʒ���ۿۺ��޹�
// 		merchInfo.nDiscountStyle &= 0xff;
// 		//�����Ʒ���ۿ۴��������ж��Ƿ��Ѿ����������ڴ�����
// 		if ( !bStamp&&!((merchInfo.nDiscountStyle == DS_ZK || merchInfo.nDiscountStyle == DS_TZ)&& merchInfo.nMerchState == RS_DS_ZK )) {
// 			merchInfo.fActualPrice = merchInfo.fLabelPrice;
// 			merchInfo.nMerchState = RS_NORMAL;
// 		}
// 		merchInfo.nSaleCount = 0;
// 		//m_bCanChangePrice = (merchInfo.nManagementStyle != MS_DP);2013-1-21 11:25:41 proj2013-1-17�Ż� saleitem ��ȥ��saleimpl��Ʒ����//aori �������Ա�� 2011-9-14 9:51:39 �Ż� ������������m_bCanChangePrice���жϸ�Ϊ�Ե�ǰ��Ʒ���ж�
// 	} else 
	{
		merchInfo=SaleMerchInfo(szPLU,GetMember().szMemberCode,scantime,strOrderNo, dCondPromDis, 
								dSaleQty, strOrderOtemID, dSaleAmt, nDiscType, dOrderSalePrice);//CSaleItem saleItem(szPLU,GetMemberCode());
		
		if ( !merchInfo.m_saleItem.IsValidMerch() ) 
			return -2;
		
		if ( !merchInfo.m_saleItem.IsAllowSale() ) 
			return -3;
		//saleItem.FormatSaleMerchInfo(merchInfo);aori del 2013-1-21 11:52:27 proj2013-1-17 16:32:11 �Ż� saleitem

		m_bCanChangePrice = merchInfo.m_saleItem.CanChangePrice();	//�����Ƿ���Ը��ļ۸�
		m_bFixedPriceAndQuantity = merchInfo.m_saleItem.IsFixedPriceAndQuantity();//ȡ�����ۣ�����еĻ���//m_promotion.GetPromotionPrice(....hInfo, GetMemberLevel(), bStamp);

		if( !bHasConfirmedMemberSale && !IsMember() && merchInfo.fRetailPrice2 < merchInfo.fRetailPrice
			&& !merchInfo.bFixedPriceAndQuantity && !merchInfo.bIsDiscountLabel)
		{
			CConfirmLimitSaleDlg confirmDlg;
			confirmDlg.m_strConfirm.Format("��Ϊ��Ա��Ʒ���Ƿ�ԭ�۽��й���");
			if ( confirmDlg.DoModal() == IDCANCEL )
				return -1;
			bHasConfirmedMemberSale = true;
		}//end aori add 2010-12-25 16:01:49 �ǻ�Ա�����Ա��Ʒʱ ѯ���Ƿ�ԭ�۹��򣨻������Ա�Ź��򣩡� later//strcpy(merchInfo.szPLUInput, szPLU);//aori del optimize 2011-9-19 15:45:13 merchInfo.szPLUInput������
		
		//��ͨ��Ʒ�������뼴Ϊ������/ɨ�룻������Ʒ��Ҫ���ݵ������������� [dandan.wu 2016-5-13]
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
		
		// ��Ӫ������Ʒ������������λ��
		if (merchInfo.nManagementStyle==MS_JE && merchInfo.nMerchType== 2 && strlen(merchInfo.szPLUInput) == GetSalePos()->GetParams()->m_nMerchCodeLen)
			return -4;//zenghl 20090619
		
		// ��Ӫ������Ʒ���뷽ʽ�� ������PoolScaleMerchSaleMode
		if (merchInfo.nMerchType==5 && strlen(merchInfo.szPLUInput)==GetSalePos()->GetParams()->m_nMerchCodeLen)
		{
			CString msg;
			switch(GetSalePos()->GetParams()->m_nPoolScaleMerchSaleMode)
			{
			case 2:	//¼��SKUʱ��ʾ,����¼��
				msg.Format("����ƷΪ������Ʒ��������ֱ��������Ʒ������! \n��ɨ����ӱ�ǩ�����ᱨ������ȡ�����ӱ�ʶ!");
				CUniTrace::Trace(msg);
				if ( IDOK !=  PosMessageBox(msg,MB_OKCANCEL))
					return -4;
				break;
			case 1:	//������¼��SKU
				return -4;
			case 0:  //������
				break;
			}
		}
		if ( bStamp && merchInfo.nDiscountStyle != DS_YH ) 
		{//�������Աָ������������ӡ�����ۣ����������ݿ�ȴ�Ҳ�����Ч���õ�ӡ������//����ʾ����Ա�����ȷ��������������ӡ�����ۣ���������ۿ۴����Ļ���//ִ���ۿ۴����ۣ�����ȡ���ۼ�
			if ( IDOK != MessageBox(((CPosApp *)AfxGetApp())->GetSaleDlg()->GetSafeHwnd(), "����ƷĿǰû����Чӡ���������Ƿ��������", "����", MB_OKCANCEL | MB_ICONWARNING ) ) //aori 2012-2-29 14:53:17 ���Է���  proj 2012-2-13 �ۿ۱�ǩУ�� ����ʾ��Ӧ��ѭ��			
				return -1;
		}
		IsDiscountLabelBar = merchInfo.m_saleItem.m_bIsDiscountLabelBar;//�Ƿ��ۿ�//bPriceInBarcode=saleItem.m_bPriceInBarcode;//aori del no use//aori add �жϸ���Ʒ�Ƿ��޹�����:��������:�ж��Ƿ��޹��������Ʒ��������merchInfo.bLimitSale��Ϊ��
		if(IsDiscountLabelBar&&!GetSalePos()->GetPOSClientImpl()->checkDiscountLabel(szPLU,true)){//aori add 2012-2-16 14:45:33 proj 2012-2-13 �ۿ۱�ǩУ�� ����Ϣ ETI_DOWNLOAD_CHECK_DiscountLabel
			return -1;
		}
	}
	
	//if(merchInfo.nManagementStyle==2 && ( merchInfo.nMerchType==5||merchInfo.nMerchType==6)  &&  strlen(merchInfo.szPLUInput)==13 )//������Ӫ�������Ʒmerchtype=5 and ManagementStyle=2	
	//��Ϊ������ֱ��ȡ��������Ϊ�ֻ���������ʼ��Ϊ1  [dandan.wu 2016-4-22]
	if ( strOrderNo.IsEmpty() )
	{
		merchInfo.fSaleQuantity = 0;
	}
	else
	{
		merchInfo.fSaleQuantity = dSaleQty;
	}
	
	if (merchInfo.bFixedPriceAndQuantity)
	{//||merchInfo.bPriceInBarcode//aori del 2011-9-22 10:30:33 ){//aori add SteelYardPriceControl proj2011-8-15-2 ����ʵ�ʼ���retailpriceƫ��ϴ�ʱ ����־��
		bool bOutCircul=false;
		double fanwei,biaozhun=(IsMember()?min(merchInfo.fRetailPrice2,merchInfo.fSysPrice):merchInfo.fSysPrice),actual_Upon_Retail=merchInfo.fActualPrice-biaozhun;
		if (actual_Upon_Retail==0)
			fanwei=0;
		else if(actual_Upon_Retail>0)
			fanwei=actual_Upon_Retail-biaozhun*GetSalePos()->GetParams()->m_SteelYardPriceUpLimit/100;
			 else	
				 fanwei=-actual_Upon_Retail-biaozhun*GetSalePos()->GetParams()->m_SteelYardPriceDownLimit/100;
		if (fanwei>0&&merchInfo.ChgSteelyardPrice==0)	
		{//aori add ChgSteelyardPrice proj2011-9-6-1 �԰��������۵���Ʒ, ��POS�˲����а��Ӽ۸񱨾�  "1" -���ӿɱ��   "0" -���Ӳ��ɱ��null Ĭ��Ϊ"0"
			CString hehe, msg;
			hehe.Format("13/18����Ʒ%s ʵ��%.3f����%.3f,ԭ��%.3f,����%d,����%d,����ʽ��%d",merchInfo.szPLU,merchInfo.fActualPrice,actual_Upon_Retail,biaozhun,GetSalePos()->GetParams()->m_SteelYardPriceUpLimit,GetSalePos()->GetParams()->m_SteelYardPriceDownLimit,GetSalePos()->GetParams()->m_SteelYardPriceControl);
			CUniTrace::Trace(hehe);
			switch (GetSalePos()->GetParams()->m_SteelYardPriceControl)
			{//0-������;1-����(��������);2.����(����������)
			case 0:
				break;
			case 1:msg.Format( "����Ʒ���ۼ�%.3f����ԭ�۸�(%.3f)�ķ�Χ%s��(�ٷ�֮%d)���Ƿ�������ۣ�",merchInfo.fActualPrice,biaozhun,actual_Upon_Retail>0?"��":"��",actual_Upon_Retail>0?GetSalePos()->GetParams()->m_SteelYardPriceUpLimit:GetSalePos()->GetParams()->m_SteelYardPriceDownLimit);
				if ( IDOK !=  PosMessageBox(msg,MB_OKCANCEL))//(((CPosApp *)AfxGetApp())->GetSaleDlg()->GetSafeHwnd(),msg, "����", MB_OKCANCEL| MB_ICONWARNING|MB_DEFBUTTON2 ) ) //aori change //aori 2012-2-29 14:53:17 ���Է���  proj 2012-2-13 �ۿ۱�ǩУ�� ����ʾ��Ӧ��ѭ��			
					return -1;
				break;//aori 2012-2-29 14:53:17 ���Է���  proj 2012-2-13 �ۿ۱�ǩУ�� ����ʾ��Ӧ��ѭ��			
			case 2:msg.Format( "����Ʒ���ۼ�%.3f����ԭ�۸�(%.3f)�ķ�Χ%s��(�ٷ�֮%d)������������\n\n  �밴��ȡ����������",merchInfo.fActualPrice,biaozhun,actual_Upon_Retail>0?"��":"��",actual_Upon_Retail>0?GetSalePos()->GetParams()->m_SteelYardPriceUpLimit:GetSalePos()->GetParams()->m_SteelYardPriceDownLimit);
				//PosMessageBox(msg);//MessageBox(((CPosApp *)AfxGetApp())->GetSaleDlg()->GetSafeHwnd(),msg, "����", MB_OK| MB_ICONWARNING ) ;//aori 2012-2-29 14:53:17 ���Է���  proj 2012-2-13 �ۿ۱�ǩУ�� ����ʾ��Ӧ��ѭ��			
				while ( IDCANCEL != MessageBox(NULL,msg, "��Ʒ����", MB_OKCANCEL | MB_ICONWARNING|MB_DEFBUTTON1 ));//aori change 2012-2-29 14:53:17 ���Է���	proj 2012-2-13 �ۿ۱�ǩУ�� ����ʾ��Ӧ��ѭ��			
				return -1;
				break;
			}
		}
	}
	int nSID = GetSupperAddSID(merchInfo.szPLU, merchInfo.fActualPrice, merchInfo.nDiscountStyle,merchInfo.m_saleItem.m_rawSaleMerch.IncludeSKU,
								merchInfo.strOrderNo, merchInfo.strOrderItemID) ;//�������Ʒ�ǿ�׷�ӵ���Ʒ����ֱ�ӷ��ظ�SID����������һ��
	if (inputer==Inputer_UpLimit_simulate)
		nSID=-1;
	if ( nSID  < 0 )
	{
		nSID=merchInfo.nSID = m_vecSaleMerch.size();
		if(!m_bFixedPriceAndQuantity && strOrderNo.IsEmpty())
			merchInfo.fSaleAmt_BeforePromotion=merchInfo.fSaleAmt = 0.0f;	//��������޸���Ʒ����������18�룩�Ͳ����¼����ܽ��//aori add merchInfo.fSaleAmt 
		if(SetItemSalesMan(merchInfo))
		{
			m_vecSaleMerch.push_back(merchInfo);//aori later 2011-9-6 9:49:29 proj2011-8-30-1.1 ӡ������ �Ӷ�ȡsalemerc���StampRate��add1���ֶ�
		   	SaveItemToTemp(nSID);
		}
		else
		{
			AfxMessageBox("�����merchʱʧ��,��ϵ�ܲ�");//aori 2012-2-29 14:53:17 ���Է���  proj 2012-2-13 �ۿ۱�ǩУ�� ����ʾ��Ӧ��ѭ��			
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
// 	����µ���Ʒ������ϸ
//
//##ModelId=3E641FAC029B
int CSaleImpl::AddItem(SaleMerchInfo &newMerchInfo)
{
	newMerchInfo.nSID = m_vecSaleMerch.size();

	m_vecSaleMerch.push_back(newMerchInfo);
	//add lxg.03/3/26������ʱ��Ʒ��
	SaveItemToTemp(newMerchInfo.nSID);

	return newMerchInfo.nSID;
}

//
//	�޸���������(�ۼ�)
//
//##ModelId=3E641FAB0286
bool CSaleImpl::ModifyQuantity(int &CurSID, double/*int*/& dbInputCount, double& fSaleAmt,Inputer inputer)//aori change ��ΪnSaleCount���ܻᱻ����
{
	//SaleMerchInfo *merchInfo = GetSaleMerchInfo(nSID);//aori change limit sale
	if(CurSID>=m_vecSaleMerch.size()||CurSID<0){
		AfxMessageBox("saleID wrong when modifyQuantity");//aori 2012-2-29 14:53:17 ���Է���  proj 2012-2-13 �ۿ۱�ǩУ�� ����ʾ��Ӧ��ѭ��			
		return false;
	}//aori add 2011-3-8 10:50:36 2.55.limit--
	vector<SaleMerchInfo>::iterator merchInfo = m_vecSaleMerch.begin()+CurSID;
	if ( merchInfo != NULL && merchInfo->nMerchState != RS_CANCEL)
	{
		m_stratege_xiangou.HandleLimitSaleV3( &merchInfo,dbInputCount,TRIGGER_TYPE_NoCheck,inputer);//2013-2-28 10:41:10 proj2013-1-23-1 ������Ʒ ���� ��Ա�������޹�			

		//��Ʒ�ۼӵ�merchInfo->fSaleQuantity,��Ϊ������Ʒ��nSaleCount��¼����������nSaleCountҲ��¼���� [dandan.wu 2016-4-19]
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
		}//aori add end 2011-1-7 8:33:29 ȡ������ʱ����0�����޹�����Ʒ
		
		ReCalculate(*merchInfo);
		fSaleAmt = merchInfo->fSaleAmt;
		
		//������ϸ��־  zenghl 20090608
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
//���ݼ۸����������ĳ���������SaleAmt��SaleDiscount, SaleQuantity
//
//##ModelId=3E641FAC02D7
void CSaleImpl::ReCalculate(SaleMerchInfo &merchInfo)
{
	if(merchInfo.fManualDiscount != 0)//MarkDown/MarkUp��Ĳ�������
		return;

	//��������ۿ�,��������޸���Ʒ����������18�룩�Ͳ����¼����ܽ��
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

	//����ΪС�������λ ZouXuehai Add 20160620
	{
		ReducePrecision(merchInfo.fSaleAmt);
	}

	merchInfo.fSaleDiscount += ReducePrecision(merchInfo.fSaleAmt);
	merchInfo.fSaleAmt_BeforePromotion = merchInfo.fSaleAmt; 
	if(merchInfo.bMemberPromotion && merchInfo.nMerchState != RS_NORMAL && merchInfo.nMerchState != RS_CANCEL && !merchInfo.bIsDiscountLabel)
	{//���¼����Ա�������ۿ�
		//merchInfo.fMemSaleDiscount = (merchInfo.fLabelPrice - merchInfo.fZKPrice) * (merchInfo.bFixedPriceAndQuantity ? merchInfo.fSaleQuantity : merchInfo.nSaleCount);
		//zenghl@nes.com.cn 20090429
	}
}

//
//	�޸���Ʒ�ۼ�
//
//##ModelId=3E641FAB0254
double CSaleImpl::ModifyPrice(int nSID, double fActualPrice)//aori 2011-9-2 17:29:03 ��Ʒ�۸��������� �Ż�
{
	SaleMerchInfo *merchInfo = GetSaleMerchInfo(nSID);
	if ( merchInfo == NULL )
		return 0.0f;

	//ֻ�зǵ�Ʒ�ࣨ�����������ܻ��õ��޸ļ۸�
	//�޸ĵļ۸����ʵ�ۼۣ�ϵͳ�۸������ݿ�ļ۸����ǲ�����
	//��Ʒ�ı�ǩ����ϵͳ��һ��
	//�ǵ�Ʒ��ı�ǩ��Ϊ����Ա¼��ļ۸�
	merchInfo->fActualPrice = fActualPrice;
	if ( merchInfo->nManagementStyle != MS_DP ) 
	{
		merchInfo->fLabelPrice = fActualPrice;
	}
	WriteSaleItemLog(*merchInfo,0,2);//aori add 2011-5-13 10:53:09 proj2011-5-11-2�����۲�ƽ5-9���ƴ�3: ��־���� writelogд��dll// ����־ zenghl 20090608

	ReCalculate(*merchInfo);
	
	return merchInfo->fSaleAmt;
}

//	����SIDȡ������ϸ
//##ModelId=3E641FAB022C
SaleMerchInfo * CSaleImpl::GetSaleMerchInfo(int nSID)
{
	if(nSID>=m_vecSaleMerch.size()||nSID<0)
		return NULL;//aori add 2012-3-15 11:02:24 proj 2012-3-5 ��Ϣ����bug Ǳ��bug
	return &m_vecSaleMerch[nSID];
}

// �ж�ĳ����Ʒ�Ƿ��ǿ�׷�ӵģ����򷵻ؽ�Ҫ׷�ӵ�nSID��
// �ж����������ڡ���Ʒ���ۿ۷�ʽ���۸�һ�£�(�����ۿ۱�ǩ)�����ǲ�����Ʒ��û�����ϲ��ұ����������������Ʒ�����ظ���Ʒ��SID
//##ModelId=3E641FAB01F0
//[in]:strOrderID : ������
//[in]:strOrderItemID:��Ʒ�ڶ����е�ItemID��saleorderItem����ItemID  ֻ�ж�����Ʒ���У�
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

//�ֶ�����ĳ�����һ���ֶ����ۣ����������ٲ��������κδ������ۿۡ���ϣ�
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
		//������ֶ����ۣ���������������ʱ��������ۿ���/�ۿۼۣ�
		merchInfo->fZKPrice = dRate;
		merchInfo->nLowLimit = SDS_RATE;
		
		//merchInfo->fManualDiscount = merchInfo->fSaleAmt * (1-dRate/100);//markdown
		double fSaleAmt = (merchInfo->fSaleAmt * dRate)/100.0;
		ReducePrecision(fSaleAmt);

		merchInfo->fSaleDiscount += merchInfo->fSaleAmt - fSaleAmt;
		merchInfo->fManualDiscount = merchInfo->fSaleAmt - fSaleAmt;//fManualDiscount>0:markdown,fManualDiscount<0:MarkUp [dandan.wu 2017-11-1]

		//�������������� [dandan.wu 2016-1-24]
//		ReducePrecision(merchInfo->fSaleDiscount);
//		ReducePrecision(merchInfo->fManualDiscount);
		merchInfo->fSaleAmt = fSaleAmt;
		merchInfo->fSaleAmt_BeforePromotion=merchInfo->fSaleAmt;//aori add 2012-2-7 16:11:04 proj 2.79 ���Է�������������ʱ ������ �� fSaleAmt_BeforePromotion

		//merchInfo->nDiscountStyle |= DS_MANUAL;
		merchInfo->nDiscountStyle &= DS_MANUAL; //Ӧ�ø�Ϊ��λ�룬0xff&�κ�һ����=������[modified by dandan.wu 2016-2-25]
		
		//delete by wjx
		// 	//ǰ̨�����ۿۣ���Ʒʵ�ʼ۸�ı� [dandan.wu 2016-2-25]
		// 	merchInfo->fActualPrice = (merchInfo->fActualPrice*fDiscount)/100.0;
		// 
		// 	//ǰ̨�����ۿۣ���Ʒʵ������۸�ı� [dandan.wu 2016-2-25]
		// 	merchInfo->fActualBoxPrice = (merchInfo->fActualBoxPrice*fDiscount)/100.0;
		// 
		// 	//��¼�ֹ��ۿ� [dandan.wu 2016-2-25]
		// 	merchInfo->fManualDiscount = fDiscount;
		
		merchInfo->nMerchState = RS_DS_MANUAL;
		merchInfo->nDiscountID = nDiscountID;
	}

	return true;
}


//======================================================================
// �� �� ����SetMemberCode
// �������������ñ������۵Ļ�Ա��Ϣ
// ���������
// ���������bool
// �������ڣ�00-2-21
// �޸����ڣ�2009-4-1 ������
// ��  �ߣ�***
// ����˵�������ݿ����޸� Sale_Temp
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

	//��¼
	CPosClientDB* pDB = CDataBaseManager::getConnection(true);
	if ( !pDB ) {
		CUniTrace::Trace("CSaleImpl::SetMemberCode");
		CUniTrace::Trace(MSG_CONNECTION_NULL, IT_ERROR);
		return-3;//aori change 2011-2-23 10:19:34//aori change 2011-3-27 13:24:11 socket �Ż�	vss2.55.0.15 ok11 CSaleImpl::SetMemberCode ����ֵҪ����
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
		member=member.Left(indexs);//��Ա��,ȥ��=����Ķ���
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
		pDB->Execute2(strSQL);//aori change 2011-3-22 16:51:36 �Ż�clientdb ��ɾ��execute2
	
	CDataBaseManager::closeConnection(pDB);
	//д��־
	CString str_logBuf;
	str_logBuf.Format("��Ա��ˢ���ɹ�,��Ա����:%s",m_MemberImpl.m_Member.szMemberCode);
	GetSalePos()->GetWriteLog()->WriteLog(str_logBuf,4);//[TIME OUT(Second) 0.000065]
	//get remove memberscore zenghl@nes.com.cn 20090428
	int iReturn=0,nUseRemoteMemScore=GetSalePos()->m_params.m_nUseRemoteMemScore;//�Ƿ��ȡԶ�̻���
	if(strlen(m_MemberImpl.m_Member.szMemberCode)>1){//aori change 2011-3-28 16:29:52 CSaleImpl::SetMemberCode 
		switch(nUseRemoteMemScore){
			case 2:		
				iReturn=GetSalePos()->GetPosMemberInterface()->ReadMembershipPointFromST(m_MemberImpl.m_Member.szMemberCode);
				break;
			case 1: //��ȡ��ԱԶ��ʵʱ���� 
				GetSalePos()->GetWriteLog()->WriteLog("��ʼ��ȡ��Ա����1",3);//[TIME OUT(Second) 0.000065]
				iReturn=GetSalePos()->GetPosMemberInterface()->ReadMemberCardInfo(m_MemberImpl.m_Member.szMemberCode,nUseRemoteMemScore)?1:-1;
				break;
			case 3: //��ȡ��ԱԶ��ʵʱ���ֺ�st����  mabh 
				GetSalePos()->GetWriteLog()->WriteLog("��ʼ��ȡ��Ա����3",3);//[TIME OUT(Second) 0.000065]
				iReturn=GetSalePos()->GetPosMemberInterface()->ReadMemberCardInfo(m_MemberImpl.m_Member.szMemberCode,nUseRemoteMemScore)?1:-1;
				break;
			case 4: //��ȡ��Աsocketʵʱ����  mabh 
				GetSalePos()->GetWriteLog()->WriteLog("��ʼ��ȡ��Ա����4",3);//[TIME OUT(Second) 0.000065]
				iReturn=GetSalePos()->GetPosMemberInterface()->ReadMemberCardInfo_socket(m_MemberImpl.m_Member.szMemberCode,nUseRemoteMemScore)?1:-1;
				break;

			case 0: //��ȡ��Ա���ػ��� //mabh 
				return 1;
		}
	}
	if (iReturn!=1)//��ȡ��Ա����ʧ��
	{
		//ClearCustomerInfo();//aori add 2011-11-21 13:55:42 ��Ա�����볬ʱ �޷�Ӧ	
		return -2;	
	}*/
//	return 1;
}

//���ص�ǰ��������ˮ
//##ModelId=3E641FAB0177
int CSaleImpl::GetSaleID()
{
	return m_nSaleID;
}
//���ص�ǰ��������ˮ
//##ModelId=3E641FAB0177
SYSTEMTIME CSaleImpl::GetSaleDT()
{
	return m_stSaleDT;
}


//======================================================================
// �� �� ����InitialNextSale()
// ������������ʼ����һ������
// ���������
// ���������bool
// �������ڣ�00-2-21
// �޸����ڣ�2009-4-1 ������
// �޸����ڣ�2009-4-6 ������ ɾ��������ʱ��
// ��  �ߣ�***
// ����˵�������ݿ����޸� Sale_Temp
//======================================================================
void CSaleImpl::InitialNextSale(SaleFinishReason FinishReaseon,int saleid)
{//aori change 2011-6-10 11:09:25 proj2011-6-10-1:���ʹ���db����

	Clear();
	m_nSaleID=m_pADB->GetNextSaleID();	
	m_vecSaleMerch.clear();//	m_VectLimitInfo.clear();//m_vecBeforReMem.clear();//aori add 2012-1-19 18:29:09proj��Ҵ��� �� �Ⱥ�ˢ��Ա����һ�� ���� ����һ�� ��Ȼ���� ����
	m_ServiceImpl.Clear();
	m_nDiscountStyle = SDS_NO;
	m_nMemDiscountStyle = SDS_NO;
	m_fDiscount = 0.0;
	m_fMemDiscount = 0.0;
	m_fDiscountRate = 100.0;
	m_fMemDiscountRate = 100.0;
	m_bCanChangePrice = false;
	//��ʼ���̱���
	//m_EorderID = "";
	//m_EMemberID = "";
	//m_EMOrderStatus = 0;
	ClearCashier(m_tSalesman);
	GetLocalTimeWithSmallTime(&m_stSaleDT);
	memcpy(&m_stCheckSaleDT, &m_stSaleDT, sizeof(SYSTEMTIME));
	memcpy(&m_stEndSaleDT, &m_stSaleDT, sizeof(SYSTEMTIME));//TODO: ���浽��ʱ��������
	CPromotionInterface *pPromotion=GetSalePos()->GetPromotionInterface();//������ʼ��//int iPromotionReturn=0;CString strPromotionMsg="";//aori del nouse optimize 2011-10-10 11:31:11 nouse
	if (pPromotion!=NULL&&pPromotion->m_bValid)
	{
		pPromotion->Init();
	}

//*******************************************
	//if(FinishReaseon==sale)DelTempSaleDB();2013-2-4 15:48:20 proj2013-1-17 �Ż� saleitem ��������Ʒ
	CAutoDB db(true);
	CString strSQL;
	switch(FinishReaseon)
	{//2013-2-4 17:17:44 proj2013-1-17-4 �Ż� saleitem ȡ����ͬ��Ʒʱsql����
		case SaleFinish_OnInputFirstMerch:
			strSQL.Format("delete from Sale_Temp where HumanID=%d ", GetSalePos()->GetCasher()->GetUserID());
			db.Execute(strSQL);
			strSQL.Format("INSERT INTO Sale_Temp(SaleDT,HumanID,MemberCode,Shift_No,MemCardNO,MemCardNOType,EOrderID,EMemberID,EMOrderStatus) VALUES"
				"('%s',%d,%s,%d,'%s',%d,'%s','%s','%d')", GetFormatDTString(m_stSaleDT), 
				GetSalePos()->GetCasher()->GetUserID(),GetFieldStringOrNULL(m_MemberImpl.m_Member.szMemberCode),m_uShiftNo,m_MemberImpl.m_Member.szInputNO,m_MemberImpl.m_Member.nCardNoType,m_EorderID,m_EMemberID,m_EMOrderStatus);
			db.Execute(strSQL);//aori change 2011-3-22 16:51:36 �Ż�clientdb ��ɾ��execute2

			break;
		case SaleFinish_CancelSale:
			m_nSaleID--;
			DelTempSaleDB();
			ClearCustomerInfo();
			GetSalePos()->GetPaymentInterface()->ArrayBill.RemoveAll(); //thinpon. ��Ҫǩ������֧����ʼ������
			break;
		case SaleFinish_poweron:
				if ( HasUncompletedSale() ) 
				{
					LoadUncompleteSale();
					//saledlg.m_uShiftNo=m_uShiftNo;2013-2-16 14:59:39 proj2013-2-16-1 �Ż� �±����ۻ�ȡδ������۵İ��
					//thinpon. 2008.08.19   Ϊ���±���ˮ��1����־��Ϊ��
					//m_bNeedAutoAdd = TRUE;//FALSE aori optimize 2011-10-10 13:28:07 ��Ϊbool �����ơ��߼�ͬʱȡ��
				}
				else 
				{
					GetShiftNO();//saledlg.m_uShiftNo = GetShiftNO();2013-2-16 14:59:39 proj2013-2-16-1 �Ż� �±�����
					UpdateShiftBeginTime();
					ClearCustomerInfo();
					GetSalePos()->GetPaymentInterface()->ArrayBill.RemoveAll();	//thinpon. ��Ҫǩ������֧����ʼ������
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
	strLogBuf.Format("���ۿ�ʼ ��ˮ:%d ����:%d ���:%d �ŵ�:%d pos��:%d",m_nSaleID,
		GetSalePos()->GetCasher()->GetUserID(),m_uShiftNo,GetSalePos()->GetParams()->m_nOrgNO,
		GetSalePos()->GetParams()->m_nPOSID);
	GetSalePos()->GetWriteLog()->WriteLog(strLogBuf,1,POSLOG_LEVEL1);	
}
void CSaleImpl::DelTempSaleDB()
{//2013-2-4 15:48:20 proj2013-1-17 �Ż� saleitem ��������Ʒ
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
// �� �� ����SetCustomerChar
// �������������ñ������۵Ĺ˿�����
// ���������
// ���������bool
// �������ڣ�00-2-21
// �޸����ڣ�2009-4-1 ������
// ��  �ߣ�***
// ����˵�������ݿ����޸� Sale_Temp
//======================================================================
void CSaleImpl::SetCustomerChar(const char *szCustomerCharCode)
{
	m_strCustomerCharacter = szCustomerCharCode;

	//��¼
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
	pDB->Execute2(strSQL);//aori change 2011-3-22 16:51:36 �Ż�clientdb ��ɾ��execute2
	CDataBaseManager::closeConnection(pDB);
}

//��ʼ��SaleImpl��ֻ�����һ��
//======================================================================��
// �� �� ����Initialize()
// ����������
// ���������
// ���������bool
// �������ڣ�00-2-21
// �޸����ڣ�2009-4-2 ������
// ��  �ߣ�***
// ����˵������������޸�,��������
//======================================================================/
void CSaleImpl::Initialize()
{
	m_pADB = &GetSalePos()->m_pADB;
	m_pParam = GetSalePos()->GetParams();
	m_nSaleID = m_pADB->GetNextSaleID();
	if ( m_nSaleID == 1 )
		m_nSaleID = GetSalePos()->GetParams()->m_nInitSaleID;
	m_PayImpl.Initialize();
	//<--#������ýӿ�#
	m_ServiceImpl.Initialize();
	if(m_ServiceImpl.HasMerchSVConfig())
		GetSalePos()->GetPaymentInterface()->m_ordinaryMarchMapPS=
			m_ServiceImpl.DelPosMobilePaymentStyle(&GetSalePos()->GetPaymentInterface()->m_ordinaryMarchMapPS);
	m_MemberImpl.Clear();

	//***************************************************
	//������ʼ��
    CPromotionInterface *pPromotion=GetSalePos()->GetPromotionInterface();
	int iPromotionReturn=0;
	CString strPromotionMsg="";
	//���ز���˵��(������ʾ2��������ʾ3����������1��������ʾ-1)
	if (pPromotion!=NULL&&pPromotion->m_bValid){
		pPromotion->Init();
	}
	m_uShiftNo=GetShiftNO();
	GetSalePos()->GetPaymentInterface()->ArrayBill.RemoveAll();

	//��ʼ���̱���
	m_EorderID = "";
	m_EMemberID = "";
	m_EMOrderStatus = 0;

	m_bSaveDataSucess=false;
}
void CSaleImpl::GetRtnID()
{
	///���˻��õ�,Ϊ�˾��������˻����غ�
	SYSTEMTIME stNow;
	GetLocalTimeWithSmallTime(&stNow);
	m_nRtnID = m_pADB->GetNextReturnID(&stNow);
}




//======================================================================
// �� �� ����Save()
// ����������������������(�����������ۡ���ϸ������,֧����Ϣ����ӡСƱ)
// ���������
// ���������void
// �������ڣ�2009-4-6 
// �޸����ڣ�
// ��  �ߣ�������
// ����˵�������Ӵ�����Ϣ����
//======================================================================
bool CSaleImpl::Save()
{
	GetSalePos()->GetWriteLog()->WriteLog("[FUN BEGIN: CSaleImpl::Save()]",3,POSLOG_LEVEL3); //���24Сʱ���⣬���������Ҫ���»��ʱ��//if(!IsInSameDay(m_stEndSaleDT, m_stSaleDT))
	GetLocalTimeWithSmallTime(&m_stEndSaleDT);
	if(!IsInSameDay(m_stEndSaleDT, m_stSaleDT)&&!m_pADB->IsSaleDataExist(&m_stEndSaleDT))	
		m_nSaleID = m_pADB->GetNextSaleID(&m_stEndSaleDT);
	if (abs(GetSalePos()->GetPromotionInterface()->m_fDisCountAmt)>=0.1)
		GetSalePos()->GetPromotionInterface()->WritePromotionLog(m_nSaleID);//д��������//aori move from below 2011-8-7 8:38:11  proj2011-8-3-1 �� mutex���ж�����c//GetSalePos()->GetWriteLog()->WriteLog("[FUN TRACE: CSaleImpl::Save()]->call IsSaleDataExist() over",3,POSLOG_LEVEL4); 	//aori del ������־//CUniTrace::Trace("--------���浽��ʽ��ǰ ��������:--!!!---------");GetSalePos()->GetPromotionInterface()->WritePromotionLog(m_nSaleID);CUniTrace::Trace("----------------------------------------------");//aori add 2011-8-3 17:03:07 proj2011-8-3-1 ���۲�ƽ �������� ��ʱ�� ����־�� 
	CPosClientDB *pDB = CDataBaseManager::getConnection(true);	
	if ( !pDB ) 
	{
		GetSalePos()->GetWriteLog()->WriteLog("[FUN TRACE: CSaleImpl::Save()]->getConnection ʧ��->return false ",3,POSLOG_LEVEL1); 
		return false;
	}
	//GetSalePos()->GetWriteLog()->WriteLog("[FUN TRACE: CSaleImpl::Save()]->getConnection success->return true ",3,POSLOG_LEVEL4); 
	pDB->BeginTrans();
	bool bRet = false;
	if ( m_PayImpl.SavePayment(pDB, m_stEndSaleDT, m_nSaleID, GetBNQTotalAmt()) )
	{
		GetSalePos()->GetWriteLog()->WriteLog("[FUN TRACE: CSaleImpl::Save()]->֧����Ϣ����ɹ�",3,POSLOG_LEVEL3);//normal
		//if(m_MemberImpl.Save(pDB, GetTotalAmt())){
		//	GetSalePos()->GetWriteLog()->WriteLog("[FUN TRACE: CSaleImpl::Save()]->��Ա��Ϣ�������",3,POSLOG_LEVEL3);//normal
			if ( Save(pDB, m_nSaleID, SS_NORMAL) )
			{
				GetSalePos()->GetWriteLog()->WriteLog("[FUN TRACE: CSaleImpl::Save()]->������Ʒ��Ϣ����ɹ�",3,3);//normal
				if (1==GetSalePos()->GetPromotionInterface()->SavePromotion(m_nSaleID,m_stEndSaleDT,pDB))
				{//pDB,aori change 2011-8-7 8:38:11  proj2011-8-3-1 B sql��ס���������Ᵽ�������Ϣ{GetSalePos()->GetWriteLog()->WriteLog("[FUN TRACE: CSaleImpl::Save()]->���ô�����Ϣ�������",3,POSLOG_LEVEL3);//normal
					bRet = true;//�������ֲ�Ϊ0ʱ���ۼ���Ա����//����ʼSaleID
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
		GetSalePos()->GetWriteLog()->WriteLog("�������ݱ���ɹ�",3,POSLOG_LEVEL1);//normal
	} 
	else 
	{ 
		pDB->RollbackTrans();	
		CDataBaseManager::closeConnection(pDB);	
		GetSalePos()->GetWriteLog()->WriteLog("�������ݱ���ʧ��",5,POSLOG_LEVEL1);GetSalePos()->GetWriteLog()->WriteLog("[FUN EXIT: CSaleImpl::Save()]->bRet=false->����ֵ:false",3,POSLOG_LEVEL1); //CUniTrace::Trace("--------���浽��ʽ��� ��������:--!!!---------");GetSalePos()->GetPromotionInterface()->WritePromotionLog(m_nSaleID);CUniTrace::Trace("----------------------------------------------");//aori add 2011-8-3 17:03:07 proj2011-8-3-1 ���۲�ƽ �������� ��ʱ�� ����־�� 
		return false;
	}
	bool isEMSale= IsEMSale();
	m_bSaveDataSucess=true;//�������ֲ�Ϊ0ʱ���ۼ���Ա���� //mabh
	if (GetSalePos()->GetPromotionInterface()->m_nHotReducedScore>0)
	{
		GetSalePos()->GetWriteLog()->WriteLog("���ú���������",3,POSLOG_LEVEL1);//normal
		//Checkbonussales();
		//Getbonusvalid();
		GetSalePos()->GetPosMemberInterface()->SetSaleImpl(this);
		if (1== GetSalePos()->GetPosMemberInterface()->UpdateMemberScore(m_nSaleID)) 
			GetSalePos()->GetWriteLog()->WriteLog(CString("����charcode")+((upd_CharCode() == 1)?"�ɹ�":"ʧ��"),3,POSLOG_LEVEL2);
	}//˰�ص���//aori begin 2011-12-30 14:42:38 proj 2011-12-22-1 combine 2.75 with 2.71 saleimpl
	if (GetSalePos()->GetParams()->m_nPrintTaxInfo == 1)
	{
 		GetSalePos()->GetWriteLog()->WriteLog("��ʼ˰�ط�Ʊ����",3);//д��־
		CTaxInfoImpl *TaxInfoImpl=GetSalePos()->GetTaxInfoImpl();
		TaxInfoImpl->SetSaleImpl(this);
		TaxInfoImpl->SetPayImpl(&m_PayImpl);
		TaxInfoImpl->SaleTaxBilling();
		GetSalePos()->GetWriteLog()->WriteLog("����˰�ط�Ʊ����",3);//д��־
	}//aori end 2011-12-30 14:42:38 proj 2011-12-22-1 combine 2.75 with 2.71 saleimpl
	//����ʵʱ���ֵ���
	//GetSalePos()->GetWriteLog()->WriteLog("��ʼ����ʵʱ���ֵ���0",3);//д��־
	if (GetSalePos()->GetParams()->m_bRealtimeJF == 1 && strlen(m_MemberImpl.m_Member.szMemberCode) > 1)
	{
 		GetSalePos()->GetWriteLog()->WriteLog("��ʼ����ʵʱ���ֵ���",3);//д��־
		//RealtimeJF  *m_RealtimeJF = GetSalePos()->GetRealtimeJF() ;
		//bool rtn = m_RealtimeJF->UpRealtimeJF(m_nSaleID);
		CString saledate=::GetFormatDTString(m_stEndSaleDT,true);		
		bool rtn = m_MemberImpl.RealtimeScore(saledate,m_nSaleID);
		if (rtn)
		{
 			GetSalePos()->GetWriteLog()->WriteLog("����ʵʱ���ֳɹ���",3);//д��־
		}
		else
		{
 			GetSalePos()->GetWriteLog()->WriteLog("����ʵʱ����ʧ�ܣ�",3);//д��־
			//m_RealtimeJF->Upd_Saleinfo(m_nSaleID,0,0);
			//m_MemberImpl.UpdSaleScore(m_nSaleID,0,0);
		}
		GetSalePos()->GetWriteLog()->WriteLog("��������ʵʱ���ֵ���",3);//д��־
	}

	//���µ��̻�Ա����״̬
	//m_pSaleImpl->m_EorderID = "1110";m_pSaleImpl->m_EMeberID = "123456";//test

	CString msg;
	msg.Format("�ж��Ƿ���̽���:m_EorderID=%s, m_EMemberID=%s",m_EorderID,m_EMemberID);
	CUniTrace::Trace(msg);

	if (isEMSale) 
	{
			//�ϴ����̶�㶩��
		if (!m_PayImpl.CheckDmalPay() && GetSalePos()->GetParams()->m_bDmallOnlinePay)
		{
			//��¼��־
			GetSalePos()->GetWriteLog()->WriteLog("�������ݴ���:��ʼ",4,POSLOG_LEVEL2);//[TIME OUT(Second) 0.000065]
			CEorderinterface* m_eorderinterface = GetSalePos()->GetEorderinterface();
			bool rtn=false;
			if(m_eorderinterface!=NULL && m_eorderinterface->m_bValid)
			{
				m_eorderinterface->SetSaleImpl(this);
				GetSalePos()->GetPromotionInterface()->LoadSaleInfoEnd(GetSaleID(),true);//д��m_vecSaleMerchEnd
				try 
				{	
					m_eorderinterface->m_fofflinePayCash = 0.00;
					rtn = m_eorderinterface->Eorder_info(2,11);//���յ��̻�Ա���������������
					if (rtn)  
					{
						//if(!m_eorderinterface->m_bDowngradeFlag)
						Upd_EMorder_status(EMORDER_STATUS_CREATE,1,m_eorderinterface->m_bDowngradeFlag);
					}
						
					if (!rtn)
					{
						GetSalePos()->GetWriteLog()->WriteLog("�������ݴ���:�쳣������",4,POSLOG_LEVEL2);
					}
				}
				catch (CException* e) 
				{ CUniTrace::Trace(*e);
					e->Delete( );		
					GetSalePos()->GetWriteLog()->WriteLog("�������ݴ���:ʧ�ܣ�����",4,POSLOG_LEVEL2);
					//MessageBox( "�������ݴ���:ʧ�ܣ�����", "��ʾ", MB_OK | MB_ICONWARNING);	
				} 

			}
			else
			{
				if (m_eorderinterface==NULL)
				{
					GetSalePos()->GetWriteLog()->WriteLog("��������ģ��Ϊ�գ�����",4,POSLOG_LEVEL2);
					//MessageBox( "��������ģ��Ϊ�գ�����", "��ʾ", MB_OK | MB_ICONWARNING);	
				}
				else
				{
					GetSalePos()->GetWriteLog()->WriteLog("��������ģ�鲻���ã�����",4,POSLOG_LEVEL2);
					//MessageBox( "��������ģ�鲻���ã�����", "��ʾ", MB_OK | MB_ICONWARNING);	

				}
			}
		}

		if (GetSalePos()->GetParams()->m_bDmallOnlinePay)
		{
			GetSalePos()->GetWriteLog()->WriteLog("���֧���ɹ�״̬�ص��ӿ�:��ʼ",4,POSLOG_LEVEL2);//[TIME OUT(Second) 0.000065]
		}
		else
		{
			GetSalePos()->GetWriteLog()->WriteLog("���µ��̻�Ա����״̬�ϴ��ӿ�:��ʼ",4,POSLOG_LEVEL2);//[TIME OUT(Second) 0.000065]
		}
		CEorderinterface* m_eorderinterface = GetSalePos()->GetEorderinterface();
		bool rtn=false;
		if(m_eorderinterface!=NULL && m_eorderinterface->m_bValid)
		{
			try {
				if (GetSalePos()->GetParams()->m_bDmallOnlinePay)
				{
					rtn = m_eorderinterface->Eorder_info(2,13);//���֧���ɹ�״̬�ص��ӿ�
				}
				else
				{
					rtn = m_eorderinterface->Eorder_info(2,3);//���µ��̻�Ա����״̬�ϴ�
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
					GetSalePos()->GetWriteLog()->WriteLog("���֧���ɹ�״̬�ص��ӿ�:ʧ�ܣ�����",4,POSLOG_LEVEL2);
				}
				else
				{
					GetSalePos()->GetWriteLog()->WriteLog("���µ��̻�Ա����״̬�ϴ��ӿ�:ʧ�ܣ�����",4,POSLOG_LEVEL2);
				}
			} 
		}
		else
		{
			if (m_eorderinterface==NULL)
				GetSalePos()->GetWriteLog()->WriteLog("��������ģ��Ϊ�գ�����",4,POSLOG_LEVEL2);
			else
				GetSalePos()->GetWriteLog()->WriteLog("��������ģ�鲻���ã�����",4,POSLOG_LEVEL2);
		}
		if (GetSalePos()->GetParams()->m_bDmallOnlinePay)
		{
			GetSalePos()->GetWriteLog()->WriteLog("���֧���ɹ�״̬�ص��ӿڣ�����",4,POSLOG_LEVEL2);//[TIME OUT(Second) 0.000065]
		}
		else
		{
			GetSalePos()->GetWriteLog()->WriteLog("���µ��̻�Ա����״̬�ϴ��ӿڣ�����",4,POSLOG_LEVEL2);//[TIME OUT(Second) 0.000065]
		}
	//�������ݴ��룺����

	}
	else//m_EorderID m_EMeberID
	{
		GetSalePos()->GetWriteLog()->WriteLog("�ǵ����������ݡ�",4,POSLOG_LEVEL2);
	}

	double fSaleAmt = GetTotalAmt();
	//��Ա�����ͻ��֣�ϵͳ����m_bMemberXFSScoreΪ1ʱ��ִ�����´���
	if (GetSalePos()->GetParams()->m_bMemberXFSScore && strlen(m_MemberImpl.m_Member.szMemberCode) > 1)
	{
		CString saledate=::GetFormatDTString(m_stEndSaleDT,true);
		bool bRtn = m_MemberImpl.MemberXFSScore(saledate, m_nSaleID, fSaleAmt);
		if(bRtn)
		{
			GetSalePos()->GetWriteLog()->WriteLog("��Ա�����ͻ��ֳɹ�!", 3);
		}
		else
		{
			GetSalePos()->GetWriteLog()->WriteLog("��Ա�����ͻ���ʧ��!", 3);
		}
	}
	//��Ա�齱���ܣ�ϵͳ����m_bMemberLotteryΪ1ʱ��ִ�����´���
	if (GetSalePos()->GetParams()->m_bMemberLottery && strlen(m_MemberImpl.m_Member.szMemberCode) > 1)
	{
		bool bReturn;
		bReturn = m_MemberImpl.MemberLottery(fSaleAmt);
		if (bReturn)
		{
			GetSalePos()->GetWriteLog()->WriteLog("��Ա�齱���ܳɹ�!", 3);
		}
		else
		{
			GetSalePos()->GetWriteLog()->WriteLog("��Ա�齱����ʧ��!", 3);
		}
	}
	GetSalePos()->GetPosMemberInterface()->SavePaymentError(m_stEndSaleDT);
	GetSalePos()->m_Task.putSale(m_nSaleID, m_stEndSaleDT);
	GetSalePos()->GetWriteLog()->WriteLog("[FUN END: CSaleImpl::Save()] ����ֵ:true",3,POSLOG_LEVEL3); 
	return bRet;
}

//����֧����
//##ModelId=3E641FAB00CD
CPaymentImpl * CSaleImpl::GetPaymentImpl()
{
	return &m_PayImpl;
}
/*
	˵�������ﲢ���ϲ��ֻ�ǰѸ���ļ۸��������
	�ù��������û����ۿ���Ʒ���ݸ���Ʒ�������޵�����۸�
	��Ҫ�����û�ȡ����ĳ����Ʒ����ʱ����ͬPLU����Ʒ�ļ۸������Ϊ�ۿ۴���������������
	�������۸�Ͳ���������
	��������˼۸������������1
	û�е���������0;
	
	�����ҳ�����Ʒ���У�������������ܺ�MergeCount
*/
//##ModelId=3E641FAB0073
unsigned int CSaleImpl::MergeItems(const char *szPLU)//2013-2-27 15:30:11 proj2013-1-23-1 ������Ʒ ���� ��Ա�������޹�
{
	//�����ҳ�����δȡ���������ۿ����۵���Ʒ�������Ѿ��ۿۻ�����Ϊδ��������δ�����ۿ۵���,���ұ����ǵ�Ʒ����
	//ͬʱ�������Ʒ�����������ܺ�
	vector<SaleMerchInfo*> vSM;
	int nMergeCount = 0;//2013-2-27 15:30:11 proj2013-1-23-1 ������Ʒ ���� ��Ա�������޹�

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

	int nRestCount = nMergeCount;//2013-2-27 15:30:11 proj2013-1-23-1 ������Ʒ ���� ��Ա�������޹�
	int nMoreCount = 0;	//ÿ�����·�����ʣ����
	int nZKCount = 0;//2013-2-27 15:30:11 proj2013-1-23-1 ������Ʒ ���� ��Ա�������޹�
	int nUpLimit = vSM[0]->nUpLimit;
	short nLowLimit = vSM[0]->nLowLimit;
	double fZKPrice = vSM[0]->fZKPrice;

	SaleMerchInfo* pSMI = NULL;
	for ( vector<SaleMerchInfo*>::iterator itr = vSM.begin(); itr != vSM.end(); ++itr ) 
	{
		pSMI = *itr;
		
		if ( nLowLimit == 0 && nUpLimit == 0) 
		{
			//���û������, û������
			pSMI->fActualPrice = fZKPrice;
			pSMI->nMerchState = RS_DS_ZK;
			nZKCount += pSMI->nSaleCount;
			nRestCount -= pSMI->nSaleCount;
		} 
		else if ( nLowLimit == 0 && nUpLimit > 0) 
		{
			//���û�����ޣ�������
			//����Լ����۵�������û�г������޵Ļ�
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
			//���������û������
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
			//���������������
			//�������û�г������ޣ���������
			if ( nMergeCount < nLowLimit ) 
			{
				pSMI->fActualPrice = pSMI->fLabelPrice;
				pSMI->nMerchState = RS_NORMAL;
				nRestCount -= pSMI->nSaleCount;
			}
			else 
			{
				//����Լ����۵�������û�г������޵Ļ�
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

	//������ʣ��һ��Ļ���������¼�һ���������������޲��ҳ������޵Ĳ��ֻ�ʣ��
	if ( nMoreCount > 0 ) {
		SaleMerchInfo newSMI=*pSMI;//memcpy(&newSMI, (const void *)pSMI, sizeof(SaleMerchInfo));
		newSMI.nSaleCount = nMoreCount;
		newSMI.fActualPrice = newSMI.fLabelPrice;
		newSMI.nMerchState = RS_NORMAL;
		newSMI.fSaleAmt =newSMI.fSaleAmt_BeforePromotion= newSMI.fActualPrice * newSMI.nSaleCount;//aori add 2012-2-7 16:11:04 proj 2.79 ���Է�������������ʱ ������ �� fSaleAmt_BeforePromotion
		newSMI.fSaleDiscount = 0;
		AddItem(newSMI);
	}

	vSM.clear();
	return 1;
}

//ȡ��ʵ����������������С��1ʱ�����ܿ�Ʊ
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

//����szPLU���ۿ����ͷ��ص�һ����ͬ������ڸ��ƣ�
//ԭ��
// 1.���Ա�ȡ������Ʒ
// 2.�������Ϊӡ�������ҵ���Ӧ��ӡ����Ʒ���Ҳ����򷵻�NULL
// 3.������ۿ۱�ǩ������NULL
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
//���ػ�Ա�ļ���
//##ModelId=3E641FAB004B
unsigned char CSaleImpl::GetMemberLevel()
{
	return m_MemberImpl.m_Member.nGradeID;
}

//���ػ�Ա����
//##ModelId=3E641FAB0037
const char * CSaleImpl::GetMemberCode()
{
	return m_MemberImpl.m_Member.szMemberCode;
}

//���ػ�Ա����
//##ModelId=3E641FAA0389
const char * CSaleImpl::GetMemberName()
{
	return m_MemberImpl.m_Member.szMemberName;
}

const double CSaleImpl::GetMemberTotIntegral()
{
	return m_MemberImpl.m_Member.dTotIntegral;
}

//���ػ�Ա������
const short  CSaleImpl::GetMemberCardType()
{
	return m_MemberImpl.m_Member.nCardStyle;
}
*/

//�����ۿ��������������Ʒ�����Ӧ�ö�����������Ʒ�����ۿ۴���
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

//�ڿ�Ʊǰ������Ʒ�����Ʒ��Ҫ�ϲ��úϲ�����
//�Ӻ�߱�����Ʒ
// ������ 20090515 �޸ĺϲ������ͺϲ���Ŀ��Ϊ��������
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
				//�ϲ�֮
				double actualPrice = 0;
				itr->nSaleCount += ritr->nSaleCount;
				itr->fSaleDiscount += ritr->fSaleDiscount;
				itr->fSaleAmt += ritr->fSaleAmt;
				itr->fSaleAmt_BeforePromotion=itr->fSaleAmt;//aori add 2012-2-7 16:11:04 proj 2.79 ���Է�������������ʱ ������ �� fSaleAmt_BeforePromotion
				
				//�ϲ�fSaleQuantity����Ϊ��������fSaleQuantity�� [dandan.wu 2016-4-19]
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
				ritr->nSID = -1; //��ʶҪɾ��
				m_vecSaleMerch.erase(ritr);
				break;
			}				
		}
	} while ( ritr != m_vecSaleMerch.begin() );

	//���±��nSID
	int nSID = 0;
	for ( ritr = m_vecSaleMerch.begin(); ritr != m_vecSaleMerch.end(); ++ritr ) {
		ritr->nSID = nSID++;
	}
}

/*
	����û����˿�Ʊ�����Ѿ����й�������㣬����Ҫ�Ѹ���ϵ�������Ʒȡ����ϣ�������Ʒ�ļ۸���Ҫ���¼���
	������vector<int> nSID���У��Ա�������
*/
//======================================================================��
// �� �� ����CancelCombine
// ����������
// ���������
// ���������bool
// �������ڣ�00-2-21
// �޸����ڣ�2009-4-2 ������
// ��  �ߣ�***
// ����˵������������޸�,��Ҫ��������
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
// 			//���¼�����ϸ
// 
// 			//ȡ�����ۣ�����еĻ���
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
	//��ʼ���̱���
	m_EorderID = "";
	m_EMemberID = "";
	m_EMOrderStatus = 0;
}

//��飬���֧������Ӧ�ս���ٵĻ����Զ������ٵĲ���
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
		strBuf.Format("����Ա: %s[%s] �����ܶ� %0.2f ֧���ܶ� %.2f", 
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
// �� �� ����SaveItemToTemp
// ����������������Ʒ��Ϣ
// ���������int nSID
// ���������bool
// �������ڣ�00-2-21
// �޸����ڣ�2009-3-31 ������
// ��  �ߣ�***
// ����˵�������ݿ����޸ģ�ȥ��DiscountStyle,SalePromoID,LowLimit,UpLimit,Shift_No)
//fZKPrice �ۿۼ� DiscountPrice
//SysPrice	ԭ�ۼ�		NormalPrice
//ActualPriceϵͳ�ۼ�		RetailPrice
//SID  ����ID  SaleID
//ItemID,ScanPLU ��Ϊ��
//2015-3-24 ���������ֶ� m_EOrderID m_EMemberID m_EMOrderStatus
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
	check_lock_saleitem_temp();//aori later 2011-8-7 8:38:11  proj2011-8-3-1 A �� mutex���ж�����checkout֮�� �Ƿ������update�¼�
	try{//CUniTrace::Trace("!!!����SaleItem_Temp���в���");//aori add 2011-8-3 17:03:07 proj2011-8-3-1 ���۲�ƽ �������� ��ʱ�� ����־�� 
		CString strSQL;
		strSQL.Format("INSERT INTO SaleItem_Temp(SaleDT,HumanID,SaleID,PLU,SimpleName,"
			"ManagementStyle,NormalPrice,LablePrice,RetailPrice,SaleCount,"
			"SaleAmt,SaleDiscount,MakeOrgNO,MerchState,DiscountPrice,"
			"MerchID, SalesHumanID,SaleQuantity,ISFixedPriceAndQuantity,itemID,ScanPLU,ClsCode,DeptCode,promoID,PromoStyle, "//thinpon ,IsDiscountLabel, BackDiscount)"
			"LimitStyle,LimitQty,CheckHistory,InputSec,IncludeSKU,BoxPrice,ItemType,OrderID,CondPromDiscount,ArtID,\
			OrderItemID,ManualDiscount)"
			//aori add for limit sale syn 2010-12-24 9:44:56 �����ֶ� CheckHistory ͬ�� saleitem_temp
			// InputSec ɨ��ʱ��
			"VALUES ('%s',%d,%d,'%s','%s',%d,%.4f,%.4f,%.4f,%d,%.4f,%.4f,%d,%d,%.4f,%d,%s,%.4f,%d,%d,'%s','%s','%s',%d,%d,%d,%.4f,%d,%d,%d,%4f,%d,'%s',%.4f,'%s',\
			'%s',%.4f)",//%d to %4f2013-2-27 15:30:11 proj2013-1-23-1 ������Ʒ ���� ��Ա�������޹�
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
			
		pDB->Execute2(strSQL);//aori change 2011-3-22 16:51:36 �Ż�clientdb ��ɾ��execute2
		CDataBaseManager::closeConnection(pDB);CHECKEDHISTORY_NoCheck;
		//CUniTrace::Trace("proj 2011-8-3-1 insert into saleitem_temp");//aori add proj 2011-8-3-1 ���۲�ƽ 2011-10-27 11:33:33 ������־
		return true;
	}catch(CException* pe) {
		GetSalePos()->GetWriteLog()->WriteLogError(*pe);
		pe->Delete();
		CDataBaseManager::closeConnection(pDB);
		return false;
	}
}

//======================================================================
// �� �� ����SaveItemToTemp
// ����������������Ʒ��Ϣ
// ���������int nSID
// ���������bool
// �������ڣ�00-2-21
// �޸����ڣ�2009-4-1 ������
// ��  �ߣ�***
// ����˵�������ݿ����޸ģ�ȥ��DiscountStyle,SalePromoID,LowLimit,UpLimit,Shift_No)
//fZKPrice �ۿۼ� DiscountPrice
//SysPrice	ԭ�ۼ�		NormalPrice
//ActualPriceϵͳ�ۼ�		RetailPrice
//SID  ����ID  SaleID
//ItemID,ScanPLU ��Ϊ��   
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
		strSQL.Format("DELETE FROM SaleItem_Temp WHERE SaleDT = '%s' AND HumanID = %d",//aori Ϊʲô���ԭ�������ݸ�һ��itemid��salehumanid���� 2011-9-16 10:24:26 proj 2011-9-16-1 2011-9-16 10:26:42 ��Ա+100401��1+282701��60+103224.5 + ȡ��100401ʱ �޹���103224���� 
			strSaleDT, nHumanID);
		pDB->Execute2(strSQL);//aori change 2011-3-22 16:51:36 �Ż�clientdb ��ɾ��execute2
		int iItem=0;
		check_lock_saleitem_temp();//pDB->Execute("select ");//aori test//aori later 2011-8-7 8:38:11  proj2011-8-3-1 A �� mutex���ж�����checkout֮�� �Ƿ������update�¼�
		//CUniTrace::Trace("!!!����SaleItem_Temp���в���");//aori add 2011-8-3 17:03:07 proj2011-8-3-1 ���۲�ƽ �������� ��ʱ�� ����־�� 
		for ( vector<SaleMerchInfo>::iterator merchInfo = m_vecSaleMerch.begin(); merchInfo != m_vecSaleMerch.end(); ++ merchInfo ) 
		{
			//�ϲ���Ʒ������ӡ��Ʊ������Ʒ�������ŷ�Ʊ���浽SaleItem_Temp����Ϊilog���¼����m_vecSaleMerch��SaleItem_Temp������Ϣ [dandan.wu 2016-10-14]
			if ( GetSalePos()->m_params.m_bAllowInvoicePrint )
			{
				strSQL.Format("INSERT INTO SaleItem_Temp(SaleDT,HumanID,SaleID,PLU,SimpleName,"
					"ManagementStyle,NormalPrice,LablePrice,RetailPrice,SaleCount,"
					"SaleAmt,SaleDiscount,MakeOrgNO,MerchState,DiscountPrice,"
					"MerchID, SalesHumanID,SaleQuantity,ISFixedPriceAndQuantity,itemID,ScanPLU,ClsCode,DeptCode,promoID,PromoStyle "//thinpon ,IsDiscountLabel, BackDiscount)"
					",LimitStyle,LimitQty,CheckHistory,InputSec,IncludeSKU,BoxPrice,ItemType,OrderID,CondPromDiscount,ArtID,OrderItemID,ManualDiscount"
					",InvoiceEndNo)"//aori add for limit sale syn 2010-12-24 9:44:56 �����ֶ� CheckHistory ͬ�� saleitem_temp
					"VALUES ('%s',%d,%d,'%s','%s',%d,%.4f,%.4f,%.4f,%d,%.4f,%.4f,%d,%d,%.4f,%d,%s,%.4f,%d,%d,'%s','%s','%s',\
					%d,%d,%d,%.4f,%d,%d,%d,%.4f, %d,'%s',%.4f,'%s','%s',%.4f,'%s')",//d to f 2013-2-27 15:30:11 proj2013-1-23-1 ������Ʒ ���� ��Ա�������޹�
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
					merchInfo->strInvoiceEndNo);//aori add for limit sale syn 2010-12-24 9:44:56 �����ֶ� CheckHistory ͬ�� saleitem_temp
			}
			else
			{
				strSQL.Format("INSERT INTO SaleItem_Temp(SaleDT,HumanID,SaleID,PLU,SimpleName,"
					"ManagementStyle,NormalPrice,LablePrice,RetailPrice,SaleCount,"
					"SaleAmt,SaleDiscount,MakeOrgNO,MerchState,DiscountPrice,"
					"MerchID, SalesHumanID,SaleQuantity,ISFixedPriceAndQuantity,itemID,ScanPLU,ClsCode,DeptCode,promoID,PromoStyle "//thinpon ,IsDiscountLabel, BackDiscount)"
					",LimitStyle,LimitQty,CheckHistory,InputSec,IncludeSKU,BoxPrice,ItemType,OrderID,CondPromDiscount,ArtID,OrderItemID,ManualDiscount)"//aori add for limit sale syn 2010-12-24 9:44:56 �����ֶ� CheckHistory ͬ�� saleitem_temp
					"VALUES ('%s',%d,%d,'%s','%s',%d,%.4f,%.4f,%.4f,%d,%.4f,%.4f,%d,%d,%.4f,%d,%s,%.4f,%d,%d,'%s','%s','%s',\
					%d,%d,%d,%.4f,%d,%d,%d,%.4f, %d,'%s',%.4f,'%s','%s',%.4f)",//d to f 2013-2-27 15:30:11 proj2013-1-23-1 ������Ʒ ���� ��Ա�������޹�
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
					merchInfo->strArtID,merchInfo->strOrderItemID, merchInfo->fManualDiscount);//aori add for limit sale syn 2010-12-24 9:44:56 �����ֶ� CheckHistory ͬ�� saleitem_temp
			}
	    	
			iItem=iItem+1;
			pDB->Execute2(strSQL);//aori change 2011-3-22 16:51:36 �Ż�clientdb ��ɾ��execute2

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
// �� �� ����UpdateItemInTemp
// ����������������ʱ��������
// ���������int nSID
// ���������bool
// �������ڣ�00-2-21
// �޸����ڣ�2009-4-1 ������
// ��  �ߣ�***
// ����˵�������ݿ����޸� SaleItem_Temp 
//======================================================================
bool CSaleImpl::UpdateItemInTemp(int nSID)
{//�޸���ʱ���ݿ��е�����
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
	check_lock_saleitem_temp();//aori later 2011-8-7 8:38:11  proj2011-8-3-1 A �� mutex���ж�����checkout֮�� �Ƿ������update�¼�
	VERIFY(merchInfo);
	//CUniTrace::Trace("!!!����SaleItem_Temp���и���");//aori add 2011-8-3 17:03:07 proj2011-8-3-1 ���۲�ƽ �������� ��ʱ�� ����־��//2013-2-27 15:30:11 proj2013-1-23-1 ������Ʒ ���� ��Ա�������޹� 
	
	CString strSQL;
	strSQL.Format("Update SaleItem_Temp set NormalPrice=%.4f,LablePrice=%.4f,RetailPrice=%.4f,SaleCount=%d, \
		SaleAmt=%.4f,SaleDiscount=%.4f,MakeOrgNO=%d,MerchState=%d,DiscountPrice=%.4f,MerchID=%d,SalesHumanID=%s,\
		LimitStyle=%d,LimitQty=%.4f ,CheckHistory=%d ,PromoID = %d \
		, PromoStyle = %d,CondPromDiscount=%.4f \
		,ManualDiscount = %.4f\
		,SaleQuantity = %.4f"//�����ֶ�SaleQuantity����Ϊ������Ʒ�����������С���㣬�������Ҫ����[dandan.wu 2016-4-15]
		//aori add 2011-3-1 10:01:40 bug:�޹�styleû�м�¼������
		//", SalesHumanID = %d "//aori add 2011-3-1 16:49:16 bug���޹��� makeOrgNO,PromoStyle,promid,SalesHumanID û�ǵ�����  
		
		"WHERE  HumanID = %d and ItemID = %d ", //SaleDT = '%s' AND
		merchInfo->fSysPrice, merchInfo->fLabelPrice,merchInfo->fActualPrice,
		merchInfo->nSaleCount,merchInfo->fSaleAmt,merchInfo->fSaleDiscount, 
		merchInfo->nMakeOrgNO, merchInfo->nMerchState, merchInfo->fZKPrice,
		merchInfo->nMerchID,
		GetFieldStringOrNULL(merchInfo->tSalesman.nHumanID, INVALIDHUMANID),
		merchInfo->LimitStyle,merchInfo->LimitQty,merchInfo->CheckedHistory,merchInfo->nPromoID,//aori add 2010-12-29 15:52:44 ������ �ָ��޹���Ϣ  limit sale syn 2010-12-24 9:44:56 �����ֶ� CheckHistory ͬ�� saleitem_temp
		merchInfo->nPromoStyle,merchInfo->dCondPromDiscount,	//aori add 2011-3-1 10:01:40 bug:�޹�styleû�м�¼������
		//merchInfo->nPromoID,//2011-3-1 16:49:16 bug���޹��� makeOrgNO,PromoStyle,promid,SalesHumanID û�ǵ����� 
		merchInfo->fManualDiscount,
		merchInfo->fSaleQuantity,//�����ֶ�SaleQuantity����Ϊ������Ʒ�����������С���㣬�������Ҫ����[dandan.wu 2016-4-15]
		 GetSalePos()->GetCasher()->GetUserID(),nSID);//GetFormatDTString(m_stSaleDT),
	TRACE(strSQL);
	//CUniTrace::Trace("proj 2011-8-3-1 update saleitem_temp");//aori add proj 2011-8-3-1 ���۲�ƽ 2011-10-27 11:33:33 ������־
	pDB->Execute2(strSQL);//aori change 2011-3-22 16:51:36 �Ż�clientdb ��ɾ��execute2
	//AfxMessageBox((LPCSTR)strSQL, MB_ICONSTOP | MB_YESNO);
	CDataBaseManager::closeConnection(pDB);
	return true;
}

bool CSaleImpl::UpdateItemInTemp_scantime(int nSID,int p_scantime)
{//�޸���ʱ���ݿ��е�����
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
	pDB->Execute2(strSQL);//aori change 2011-3-22 16:51:36 �Ż�clientdb ��ɾ��execute2
	CDataBaseManager::closeConnection(pDB);
	return true;
}


//##ModelId=3E641FAA034D
//ɾ�������û���������ʱ���� 2009-05-11
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
		//ɾ�������û���������ʱ����
		strSQL.Format("DELETE FROM Sale_Temp WHERE HumanID != %d", GetSalePos()->GetCasher()->GetUserID());
		pDB->Execute2(strSQL);//aori change 2011-3-22 16:51:36 �Ż�clientdb ��ɾ��execute2
		strSQL.Format("DELETE FROM SaleItem_Temp WHERE HumanID != %d", GetSalePos()->GetCasher()->GetUserID());
		check_lock_saleitem_temp();//aori later 2011-8-7 8:38:11  proj2011-8-3-1 A �� mutex���ж�����checkout֮�� �Ƿ������update�¼�
		pDB->Execute2(strSQL);//aori change 2011-3-22 16:51:36 �Ż�clientdb ��ɾ��execute2
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
// �� �� ����LoadUncompleteSale
// ����������װ��δ��ɵ�����
// ���������int nSID
// ���������bool
// �������ڣ�00-2-21
// �޸����ڣ�2009-4-1 ������
// �޸����ڣ�2009-4-6 ������ �޸Ĵ�ӡ����
// ��  �ߣ�***
// ����˵�������ݿ����޸� SaleItem_Temp
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
	//��Աƽ̨���������Ա����
	strSQL.Format("SELECT SaleDT,MemberCode,CharCode,Shift_no ,EOrderID,EMemberID,EMOrderStatus,MemCardNO,MemCardNOType "
		          "  FROM Sale_Temp WHERE HumanID = %d", 
		GetSalePos()->GetCasher()->GetUserID());
	
	try {
		if ( pDB->Execute(strSQL, &pRecord) > 0 ) {
			pRecord->GetFieldValue((short)0, var);
			TStoST(*var.m_pdate, m_stSaleDT);

			pRecord->GetFieldValue(1, var);

			//�ǻ�Աƽ̨
			if (GetSalePos()->GetParams()->m_nUseRemoteMemScore != VIPCARD_QUERYMODE_MEMBERPLATFORM)
			{
				if(var.m_dwType != DBVT_NULL && strlen(*var.m_pstring)>0)
				{
					//m_MemberImpl.SetMember(*var.m_pstring);
					//if(SetMemberCode(*var.m_pstring,0,0,1)==1)//if(SetMemberCode(*var.m_pstring,0,0,1))aori change bug:ԭ����-1ʱ��Ϊ�ɹ� 2011-3-28 16:29:52 setmembercode �Ż�
					m_MemberImpl.QueryMember(*var.m_pstring,VIPCARD_NOTYPE_FACENO);
				}
			}
			pRecord->GetFieldValue(2, var);
			if ( var.m_dwType != DBVT_NULL ) 
				m_strCustomerCharacter = *var.m_pstring;
			else
				m_strCustomerCharacter = "";

			//thinpon ��ȡδ������۰��.
			pRecord->GetFieldValue(3, var);
			m_uShiftNo=atof(*var.m_pstring);
			//��ȡ���̶�����
			pRecord->GetFieldValue(4,var);
			if(var.m_dwType != DBVT_NULL && strlen(*var.m_pstring)>0) 
				m_EorderID = *var.m_pstring;
			else 
				m_EorderID = "";
			//��ȡ���̻�Ա��
			pRecord->GetFieldValue(5,var);
			if(var.m_dwType != DBVT_NULL && strlen(*var.m_pstring)>0)
				m_EMemberID = *var.m_pstring;
			else 
				m_EMemberID = "";
			//��ȡ���̶���״̬
			pRecord->GetFieldValue(6,var);
			if(var.m_dwType != DBVT_NULL)
				m_EMOrderStatus =var.m_chVal;
			else 
				m_EMOrderStatus = 0;

			//��Աƽ̨������
			CString m_szMemCardNO;
			pRecord->GetFieldValue(7,var);
			if(var.m_dwType != DBVT_NULL)
				m_szMemCardNO = *var.m_pstring;
			else
				m_szMemCardNO = "";

			int MemCardNOType=0;
			pRecord->GetFieldValue(8,var);
			MemCardNOType = var.m_chVal;
			//��Աƽ̨
			if (GetSalePos()->GetParams()->m_nUseRemoteMemScore == VIPCARD_QUERYMODE_MEMBERPLATFORM)
			{
				if(MemCardNOType >0 && m_szMemCardNO.GetLength()>0)
				{
					//m_MemberImpl.SetMember(*var.m_pstring);
					//if(SetMemberCode(*var.m_pstring,0,0,1)==1)//if(SetMemberCode(*var.m_pstring,0,0,1))aori change bug:ԭ����-1ʱ��Ϊ�ɹ� 2011-3-28 16:29:52 setmembercode �Ż�
					m_MemberImpl.QueryMember(m_szMemCardNO,MemCardNOType);
				}
			}
			pDB->CloseRecordset(pRecord);
			bHas = true;

			//thinpon.�ش�ӡδ���СƱ�ı�ͷ.
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
		"a.LimitStyle,isnull(b.LimitStyle,0),a.LimitQty,b.RetailPrice,a.CheckHistory "//aori add limit sale syn 2010-12-24 9:44:56 �����ֶ� CheckHistory ͬ�� saleitem_temp;//aori add	2011-1-5 8:32:58 ��rowLimitStyle
		",isnull(b.AddCol1,0),"//2011-9-13 16:10:51 proj2011-9-13-2 ӡ�� ���԰��� 62�ϱ���//aori add proj2011-8-30-1.2 ӡ������  2 �����ֶεĶ�дͬ��
		//<--#������ýӿ�
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
				short nSID = var.m_iVal;//2013-2-4 15:48:20 proj2013-1-17 �Ż� saleitem ��������Ʒ
				pRecord->GetFieldValue(1, var);
				SaleMerchInfo merchInfo(*var.m_pstring);
				merchInfo.nSID=nSID;//2013-2-4 15:48:20 proj2013-1-17 �Ż� saleitem ��������Ʒ
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
				merchInfo.fSaleAmt = merchInfo.fSaleAmt_BeforePromotion= atof(*var.m_pstring);//aori add 2012-2-7 16:11:04 proj 2.79 ���Է�������������ʱ ������ �� fSaleAmt_BeforePromotion

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
				merchInfo.nSimplePromoID = var.m_lVal;//�򵥴����Ž���
				if(var.m_dwType != DBVT_NULL)
				{
					SetSalesMan(var.m_lVal);//ӪҵԱ
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
					
				/*if (bAssign&&(iPromoStyle==4||iPromoStyle==5))//�д��� //mabh
				{	pRecord->GetFieldValue(19, var);
					fSaleAtm_temp =atof(*var.m_pstring);
					merchInfo.fSaleAmt = fSaleAtm_temp;
				}*/
			
				pRecord->GetFieldValue(20, var);
				merchInfo.szPLUInput=*var.m_pstring;

				pRecord->GetFieldValue(21, var);
				merchInfo.fMemSaleDiscount = atof(*var.m_pstring);//�ǻ�Ա�ۿ�
				merchInfo.fMemberShipPoint = atof(*var.m_pstring);//�Ǻ������ֽ���

				pRecord->GetFieldValue(22, var);
				merchInfo.szClsCode= *var.m_pstring;

				pRecord->GetFieldValue(23, var);
				merchInfo.szDeptCode= *var.m_pstring;

				pRecord->GetFieldValue(24, var);
				merchInfo.fManualDiscount = atof(*var.m_pstring);//��Ʒ�ۿ�
				merchInfo.fItemDiscount = atof(*var.m_pstring);//��Ʒ�ۿ۽���

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
				pRecord->GetFieldValue(30, var );//aori add 2010-12-24 9:44:56 �����ֶ� CheckHistory ͬ�� saleitem_temp
				merchInfo.CheckedHistory=(CHECKED_HISTORY) var.m_chVal;
				pRecord->GetFieldValue(31, var);merchInfo.StampRate=atof( *var.m_pstring);////aori add proj2011-8-30-1.2 ӡ������  2 �����ֶεĶ�дͬ��
				IsLimitSale(&merchInfo);//aori add format salemerchinfo.blimitsale
				//thinpon ��ӡ��Ʒ�� 
				((CPosApp *)AfxGetApp())->GetSaleDlg()->m_dbInputCount=merchInfo.fSaleQuantity/*nSaleCount*/;//aori add 2011-11-29 12:41:23��proj 2011-11-4-1 T1472 ֧������ʶ�� 2011-11-26 8:28:14 ���Է���	posɨ����棬������pos��ӡ����Ʒ�����ͽ��Ϊ�㡣
				((CPosApp *)AfxGetApp())->GetSaleDlg()->PrintSaleZhuHang(&merchInfo);//aori optimize 2011-10-13 14:53:19 ���д�ӡ del //if (GetSalePos()->m_params.m_nPrinterType==2)PrintBillItemSmall(merchInfo.szPLU,merchInfo.szSimpleName,merchInfo.nSaleCount,merchInfo.fActualPrice,merchInfo.fSaleAmt);
				// ����ɨ���ʱ��֧����ʱ
				secsScan = GetLocalTimeSecs();
				secsPay  = GetLocalTimeSecs();
				//<--#������ýӿ�
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
				//ɨ����Ʒʱ�� mabh
				pRecord->GetFieldValue(35, var);//nMerchType
				merchInfo.scantime=atof(*var.m_pstring);
				//����
				pRecord->GetFieldValue(36, var);
				merchInfo.IncludeSKU = var.m_lVal;
				//���
				pRecord->GetFieldValue(37, var);
				merchInfo.fActualBoxPrice = atof(*var.m_pstring);
				merchInfo.fBoxLabelPrice = merchInfo.fActualBoxPrice;

				//��Ʒ����
				pRecord->GetFieldValue(38, var);
				merchInfo.nItemType = var.m_chVal;
				//������
				pRecord->GetFieldValue(39,var);
				merchInfo.strOrderNo = *var.m_pstring;
				//��������
				pRecord->GetFieldValue(40,var);
				merchInfo.strOrderItemID = var.m_dwType != DBVT_NULL?*var.m_pstring:"";;
				//�Ƿ�����Ա�ۿ�
				pRecord->GetFieldValue(41,var);
				merchInfo.bVipDiscount = var.m_chVal;
				pRecord->GetFieldValue(42,var);
				merchInfo.strArtID=var.m_dwType != DBVT_NULL?*var.m_pstring:"";
				m_vecSaleMerch.push_back(merchInfo);//aori later 2011-9-6 9:49:29 proj2011-8-30-1.1 ӡ������ �Ӷ�ȡsalemerc���StampRate��add1���ֶ�
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
	//��ȯ������¶�ȡ
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
	//<--#������ýӿ�
	if(m_PayImpl.m_vecPay.size()==0 && m_ServiceImpl.HasSVMerch())
		m_ServiceImpl.Clear();
	//-->
	GetSalePos()->GetPromotionInterface()->SumPromotionAmt(m_nSaleID);//this->m_nSaleID);//aori change

	//�жϻ��ַ������Ƿ����Ѿ��ɹ��ύ����֧��δ����ִ�м�¼
	char shopperNo[20];
	CString czShopperNo="";
	char cshopperlog[50];
	
	GetSalePos()->GetPaymentInterface()->mobileLogFile->getItemValueByName("tomePass",shopperNo);
	czShopperNo.Format("%s",shopperNo);	
	if (GetSalePos()->GetPaymentInterface()->mobileLogFile->checkedReversal(CHECK_TYPE_ALL)){
		//���ֻ���������δִ�м�¼
		AfxMessageBox("�ֻ�֧���쳣������ɺ���������");
		
		sprintf(cshopperlog,"�������������ֻ�֧����δ�����¼��shopperNo=%s��",shopperNo);
		GetSalePos()->GetWriteLog()->WriteLog(cshopperlog,1);
	}
	//�жϻ��ַ������Ƿ����Ѿ��ɹ��ύ����֧��δ����ִ�м�¼��������

	if(m_PayImpl.m_vecPay.size() > 0 || GetSalePos()->GetPromotionInterface()->m_fDisCountAmt >0 ||
		(m_vecSaleMerch.size()>0 &&GetSalePos()->GetPaymentInterface()->mobileLogFile->checkedReversal(CHECK_TYPE_ALL)))//����ֻ���ֵ�Ƿ���Ҫ���г�������
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
	pDB->Execute2(strSQL);//aori change 2011-3-22 16:51:36 �Ż�clientdb ��ɾ��execute2

	strSQL.Format("DELETE FROM SaleItem_Temp WHERE HumanID = %d", nCashierID);
	check_lock_saleitem_temp();//aori later 2011-8-7 8:38:11  proj2011-8-3-1 A �� mutex���ж�����checkout֮�� �Ƿ������update�¼�
	pDB->Execute2(strSQL);//aori change 2011-3-22 16:51:36 �Ż�clientdb ��ɾ��execute2

	m_PayImpl.DelAllTempPayment(pDB);

	CDataBaseManager::closeConnection(pDB);*/
}

//======================================================================
// �� �� ����ReloadSale
// ����������
// ���������CPosClientDB *pDB, int nSaleID, SYSTEMTIME& stSaleDT
// ���������bool
// �������ڣ�00-2-21
// �޸����ڣ�2009-4-1 ������
// �޸����ڣ�2009-4-6 ������ �޸Ĵ�ӡ����
// 2009-4-5 �޸Ĵ�ӡ
// ��  �ߣ�***
// ����˵�������ݿ����޸� saleitem,
//======================================================================
bool CSaleImpl::ReloadSale( int nSaleID, SYSTEMTIME& stSaleDT,bool pCheckLimitSale)
{	
	CAutoDB db(true);
	CString strSql;
	CString strTableDTExt;
	
	strTableDTExt.Format("%04d%02d", stSaleDT.wYear, stSaleDT.wMonth);
	CString strSpanBegin= GetDaySpanString(&stSaleDT);
	CString strSpanEnd= GetDaySpanString(&stSaleDT, false);
	
	int col = 0;//��ȡ����������Ϣ
	
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
		db.Execute(strSql);//aori bug ??? !!! 2011-1-2 21:55:30 reloadsaleʱ����һ�������ڵļ�¼//aori change 2011-3-22 16:51:36 �Ż�clientdb ��ɾ��execute2
		
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
			//SetMemberCode(*var.m_pstring,0,0,1);//aori add 2010-12-30 21:36:47 bug �Ƚ�� ������ʱ ��Ʒ����ʾ�޹� ����ʧ�˻�Ա��ݣ� û�лָ� m_veclimitsaleinfo	
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

		// thinpon. ��ȡ��� 
		m_uShiftNo = atof(*db.GetFieldValue( col++).m_pstring);

		// ��ӡ��ͷ
		if (GetSalePos()->m_params.m_nPrinterType==2)PrintBillHeadSmall();

		//(ɨ���ʱ)��(֧����ʱ)��Ϊ��ǰ
		secsPay =secsScan = GetLocalTimeSecs();			
	} 
	else 
	{
		return false;
	}
		
	// reloadsalepromotion mabh
	GetSalePos()->GetPromotionInterface()->reloadPromotion(nSaleID,stSaleDT);
		
	//��ȡ������ϸ����ȫ��ȡ����//(ȥ�� ManualDiscount,zenghl 20090409)			/*strSql.Format( "SELECT PLU, RetailPrice, SaleCount, SaleQuantity, SaleAmt,  PromoStyle, "				"MakeOrgNO, PromoID, ManualDiscount, SalesHumanID, MemberDiscount FROM SaleItem%s WHERE SaleID = %d AND ""SaleDT >= '%s' AND SaleDT < '%s' ORDER BY ItemID", strTableDTExt, nSaleID, strSpanBegin, strSpanEnd);*/
	strSql.Format( "select a.PLU, a.RetailPrice,b.RetailPrice,"		//aori add
		" a.SaleCount, a.SaleQuantity, a.SaleAmt,  a.PromoStyle,"	//aori change saleamt to addcol1 2012-2-10 10:18:18 proj 2.79 ���Է�������������ʱ Ӧ������δ�ܱ��桢�ָ���Ʒ�Ĵ���ǰsaleAmt��Ϣ��db�����ֶα������Ϣ
		"a.MakeOrgNO, a.PromoID,a.NormalPrice, a.SalesHumanID,"
		"isnull(a.MemberDiscount,0) , isnull(b.clscode,NULL),"
		"isnull(b.deptcode,NULL),isnull(a.ScanPLU,NULL),b.merchtype "
		",isnull(a.AddCol1,0) "	//aori add 2012-2-10 10:18:18 proj 2.79 ���Է�������������ʱ Ӧ������δ�ܱ��桢�ָ���Ʒ�Ĵ���ǰsaleAmt��Ϣ��db�����ֶα������Ϣ
		",isnull(a.ManualDiscount,0) "//aori add ����addcol1�� 2012-2-10 10:18:18 proj 2.79 ���Է�������������ʱ Ӧ������δ�ܱ��桢�ָ���Ʒ�Ĵ���ǰsaleAmt��Ϣ��db�����ֶα������Ϣ		
		",a.ItemType,a.OrderID,a.CondPromDiscount,a.ArtID,a.OrderItemID,a.MemDiscAfterProm" //Bnq����ӵ��ֶ� [dandan.wu 2016-1-28]
		",isnull(a.AddCol2,''),isnull(a.AddCol3,''),isnull(a.AddCol4,'') "//<--#������ýӿ�
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
			merchInfo.fSaleAmt = atof(*db.GetFieldValue( col++).m_pstring);//aori del = merchInfo.fSaleAmt_BeforePromotion=2012-2-10 10:18:18 proj 2.79 ���Է�������������ʱ Ӧ������δ�ܱ��桢�ָ���Ʒ�Ĵ���ǰsaleAmt��Ϣ��db�����ֶα������Ϣ//SaleAmt//aori add 2012-2-7 16:11:04 proj 2.79 ���Է�������������ʱ ������ �� fSaleAmt_BeforePromotion
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
			merchInfo.szPLUInput= (const char *)*db.GetFieldValue( col++).m_pstring ;//isnull(a.ScanPLU,0)//szPLUInput//�ж��Ƿ���������Ƿ������Ʒ//&bPriceInBarcode, &bFixedPriceAndQuantity//bool  bFixedPriceAndQuantity=false,bPriceInBarcode=false;//aori  2011-8-15 10:29:27 proj2011-8-15-1 ������ƷСƱ����ӡ���ۡ�
		
			merchInfo.m_saleItem.IsMoneyMerchCode(merchInfo.szPLUInput,&merchInfo.bPriceInBarcode,&merchInfo.bFixedPriceAndQuantity);
		
			merchInfo.bIsDiscountLabel =  merchInfo.nDiscountStyle==3 ? 1:0 || strlen(merchInfo.szPLUInput)==m_pParam->m_nDiscountLabelBarLength;	 
			
			merchInfo.nMerchType = db.GetFieldValue( col++).m_chVal;//nMerchType

			//merchInfo.fSaleAmt_BeforePromotionֱ��ȡAddCol1 [dandan.wu 2016-5-23]
			merchInfo.fSaleAmt_BeforePromotion = atof(*db.GetFieldValue( col++).m_pstring);//aori add 2012-2-10 10:18:18 proj 2.79 ���Է�������������ʱ Ӧ������δ�ܱ��桢�ָ���Ʒ�Ĵ���ǰsaleAmt��Ϣ��db�����ֶα������Ϣ
			 
			//����ֹ��ۿ� [dandan.wu 2016-5-23]
			//merchInfo.fSaleAmt_BeforePromotion = atof(*db.GetFieldValue( col++).m_pstring)+merchInfo.fSaleAmt;//aori add ����Ҫaddcol1�� 2012-2-10 10:18:18 proj 2.79 ���Է�������������ʱ Ӧ������δ�ܱ��桢�ָ���Ʒ�Ĵ���ǰsaleAmt��Ϣ��db�����ֶα������Ϣ
			merchInfo.fManualDiscount = (atof)(*db.GetFieldValue( col++).m_pstring);
		
			//��Ʒ���ͺͶ�������BNQ����ӵ��ֶΣ���Ҫ��ֵ���Ա��ش�ӡʱ�����ֻ��Ͷ��� [Added by dandan.wu 2016-1-28]
			merchInfo.nItemType = db.GetFieldValue( col++).m_chVal;
			merchInfo.strOrderNo = *db.GetFieldValue( col++).m_pstring;
			merchInfo.dCondPromDiscount = (atof)(*db.GetFieldValue( col++).m_pstring);
			merchInfo.strArtID = *db.GetFieldValue( col++).m_pstring;
			merchInfo.strOrderItemID = *db.GetFieldValue( col++).m_pstring;
			merchInfo.dMemDiscAfterProm = (atof)(*db.GetFieldValue( col++).m_pstring);

			IsLimitSale( &merchInfo);//aori add 2010-12-29 16:53:52 �޹����ܲ�֧�ֹ��𡢽��  
			m_vecSaleMerch.push_back(merchInfo);//aori later 2011-9-6 9:49:29 proj2011-8-30-1.1 ӡ������ �Ӷ�ȡsalemerc���StampRate��add1���ֶ�
			
			((CPosApp *)AfxGetApp())->GetSaleDlg()->PrintSaleZhuHang(&merchInfo);//aori optimize 2011-10-13 14:53:19 ���д�ӡ del //if (GetSalePos()->m_params.m_nPrinterType==2)PrintBillItemSmall(merchInfo.szPLU,merchInfo.szSimpleName,merchInfo.nSaleCount,merchInfo.fActualPrice,merchInfo.fSaleAmt);
			
			//<--#������ýӿ�
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
			m_stratege_xiangou.checkXianGouV2(TRIGGER_TYPE_NoCheck,-1,false);//aori add bQueryBuyingUpLimit 2011-4-26 9:44:43 �޹����⣺ȡ����Ʒʱ����ѯ���Ƿ�������Ʒ
		}	
	}//end if ( pDB->Execute(strSql, &pRecord) > 0 )

	//���õ�ǰ����ˮ��Ϊ���ص���ˮ��
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

	Clear();//int bk_saleid=m_nSaleID;//aori add 2012-2-9 13:49:53 proj 2.79 ���Է������ش�ӡСƱ Ӧ��������
	
	m_nSaleID = nSaleID;//aori add 2011-10-14 10:54:21 bug �ش�ӡʱ  ���Ŵ���

	CString strTableDTExt;
	strTableDTExt.Format("%04d%02d", stSaleDT.wYear, stSaleDT.wMonth);//aori 2011-12-30 14:42:38 proj 2011-12-22-1 combine 2.75 with 2.71 saleimpl
	
	bool bRet = false;
	if ( ReloadSale(nSaleID, stSaleDT,false) ) 
	{
		if ( m_PayImpl.ReloadPayment(pDB, m_stSaleDT, m_nSaleID) ) 
		{
			//˰�ص���//aori begin 2011-12-30 14:42:38 proj 2011-12-22-1 combine 2.75 with 2.71 saleimpl
			if (GetSalePos()->GetParams()->m_nPrintTaxInfo == 1)
			{
				GetSalePos()->GetWriteLog()->WriteLog("��ʼ˰�ط�Ʊ����",3);//д��־
				CTaxInfoImpl *TaxInfoImpl=GetSalePos()->GetTaxInfoImpl();
				TaxInfoImpl->SetSaleImpl(this);
				TaxInfoImpl->SetPayImpl(&m_PayImpl);
				TaxInfoImpl->RePrtSaleTaxInfo(reprttype, m_nSaleID, strTableDTExt);
				if(reprttype == 1  || reprttype == 0)
				{
					//�ش�ӡ��Ʊ������¼
					CString strBuf;
					strBuf.Format("%d", m_nSaleID);
					m_pADB->SetOperateRecord(OPT_REPRTTAX, GetSalePos()->GetCasher()->GetUserID(), strBuf);
				}
				GetSalePos()->GetWriteLog()->WriteLog("����˰�ط�Ʊ����",3);//д��־
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

	//m_nSaleID=bk_saleid;//aori add	2012-2-9 13:49:53 proj 2.79 ���Է������ش�ӡСƱ Ӧ��������
	return bRet;
}

void CSaleImpl::PrintBillLine(const char* strbuf,int nAlign)
{	//aori add 2011-12-30 14:42:38 proj 2011-12-22-1 combine 2.75 with 2.71 saleimpl
	if (!GetSalePos()->GetPrintInterface()->m_bValid) 
	{
		//��ӡ�����������˳�
		GetSalePos()->GetWriteLog()->WriteLog("END PrintBillWhole PrintInterface::m_bValid��ֹ��ӡ",4,POSLOG_LEVEL1);//aori add 2011-8-30 10:23:53 proj2011-8-30-2 �Ż��ɶ���
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

//======================================================================��
// �� �� ����ReCalculatePromoPrice()
// ����������
// ���������
// ���������bool
// �������ڣ�00-2-21
// �޸����ڣ�2009-4-2 ������
// ��  �ߣ�***
// ����˵������������޸�,��Ҫ��������
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

//======================================================================��
// �� �� ����ReCalculatePromoPrice()
// ����������
// ���������
// ���������bool
// �������ڣ�00-2-21
// �޸����ڣ�2009-4-2 thinpon
// �޸����ڣ�2009-5-18 ZENGHL �޸� //�Ա����Ʒ��������
// �޸����ڣ�2009523 ZENGHL �޸�  ��ֹ��ˢ��Ա�� 18����ֽ����λС��
// ��  �ߣ�***
// ����˵������Ա����޸�,��Ҫ��������
//======================================================================/
void CSaleImpl::ReCalculateMemPrice(bool bNeedZeroLimitInfo){
	m_stratege_xiangou.m_vecBeforReMem=m_vecSaleMerch;//aori add 2011-5-11 9:14:43 proj2011-5-3-1:���д�ӡ���ƣ������Ա�š� ȡ����Ʒ 4 �����ȡ����Ʒʱ���۸񲻶�
	bNeedZeroLimitInfo?m_stratege_xiangou.m_VectLimitInfo.clear():1;//aori add 2011-3-3 10:39:30�޹�bug:�������벻ͬ��Ա��ʱ�۸���� //aori add bNeedZeroLimitInfo 2011-4-8 14:33:28 limit bug ȡ����Ʒ�� �޹�����//int i=0;//aori del
	for ( vector<SaleMerchInfo>::iterator merchInfo = m_vecSaleMerch.begin(); merchInfo != m_vecSaleMerch.end(); ++merchInfo )
	{
		//fManualDiscount>0:MarkDown,fManualDiscount<0:MarkUp [dandan.wu 2017-11-1]
		if(merchInfo->m_saleItem.CanChangePrice() 
			|| merchInfo->fManualDiscount != 0 
			|| merchInfo->nItemType == ITEM_IN_ORDER)//������Ʒ���޸ļ۸�
			continue; //���޸ļ۸����Ʒ(����)��ˢ�¼۸� markdwon�����Ʒ��ˢ�¼۸�
		
		merchInfo->bLimitSale=0;//aori add  2011-3-4 17:30:02 ȫ��СƱ��Ʒ bug ����Ա��==�ۿۼ�ʱ ��Ա������ȫ��СƱʱ Ӧȡ normal��
		SaleMerchInfo tmpmerch=*merchInfo;//aori add 2011-5-3 11:36:57 proj2011-5-3-1:���д�ӡ���ƣ������Ա�š� ȡ����Ʒ
		
		if ( (merchInfo->nMerchState != RS_CANCEL) && !(merchInfo->bIsDiscountLabel))
		{
			SaleMerchInfo ReCalmerchInfo(merchInfo->szPLUInput,GetMember().szMemberCode);
			ReCalmerchInfo.szPLUInput= merchInfo->szPLUInput;
            bool bFixedPriceAndQuantity=false,bPriceInBarcode=false;
			ReCalmerchInfo.m_saleItem.IsMoneyMerchCode(merchInfo->szPLUInput, &bPriceInBarcode, &bFixedPriceAndQuantity); 
  			merchInfo->nPromoStyle	=		ReCalmerchInfo.nPromoStyle	;
  			merchInfo->nMakeOrgNO	=  		ReCalmerchInfo.nMakeOrgNO	;	
  			merchInfo->nPromoID		=  		ReCalmerchInfo.nPromoID		;
			
			//aori add 2011-3-2 9:37:11 bug���ڶ��������Ա��ʱ �۸����
			merchInfo->CheckedHistory=ReCalmerchInfo.CheckedHistory;
			merchInfo->LimitStyle=ReCalmerchInfo.LimitStyle;
			merchInfo->fLabelPrice=ReCalmerchInfo.fLabelPrice;//2013-2-5 13:52:32 proj2013-1-17-5 �Ż� saleitem �ȳƺ�label�۴�
			if (ReCalmerchInfo.fActualPrice >0)
			{//&&ReCalmerchInfo.nManagementStyle!=MS_JE){ //����0����if (ReCalmerchInfo.nManagementStyle==MS_JE)//&&(ReCalmerchInfo.nMerchType==3||ReCalmerchInfo.nMerchType==6)){i++;continue;//�Խ����,������Ʒ������۴���}
				merchInfo->fActualPrice= ReCalmerchInfo.fActualPrice;
				merchInfo->fSaleAmt=bFixedPriceAndQuantity?merchInfo->fSaleAmt:ReCalmerchInfo.fActualPrice * merchInfo->fSaleQuantity/*nSaleCount*/;
				
			}
		}//	i++;	//aori del
	}//bNeedReprint?ReprintAll():1;//aori add 5/10/2011 9:18:37 AM proj2011-5-3-1:���д�ӡ���ƣ������Ա�š� ȡ����Ʒ2
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
	//���û�б�ʾ���ҵ��Ա
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
	//���û�б�ʾ���ҵ��Ա
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

//�趨ָ����ŵ����۵�ҵ��Ա
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
//�ж�ҵ��Ա����Ʒ�Ķ�Ӧ�Ƿ�Ϸ�,���ڻ��ܵ�ԭ��ͬһ����Ʒֻ�ܶ�Ӧһ��ҵ��Ա
//�ж�Ҫ�ų�nSID��Ӧ����Ʒ��
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
				strTemp.Format("��Ʒ%s�ѱ�ҵ��Ա%s����,\n�趨Ϊҵ��Ա%s����(��)\n����һ������������(��)",merchInfo->szSimpleName,merchInfo->tSalesman.szUserName,merchInfo->tSalesman.szUserName);
				if(AfxMessageBox(strTemp, MB_ICONSTOP | MB_YESNO) == IDYES)//aori 2012-2-29 14:53:17 ���Է���  proj 2012-2-13 �ۿ۱�ǩУ�� ����ʾ��Ӧ��ѭ��			
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
//�������е���Ʒ�Ļ�Ա�ۿۣ������Ʒ�д����Ͳ��ۣ���������ۿ�
bool CSaleImpl::GetMemWholeDiscount(double fMemDiscountRate, double* fMemWholeDiscount)
{
	*fMemWholeDiscount = 0;
	double fAmt = 0;
	//���û�л�Ա�ۿۣ��Ͳ�����
	if(fMemDiscountRate >= 1)
		return false;
	for ( vector<SaleMerchInfo>::iterator merchInfo = m_vecSaleMerch.begin(); merchInfo != m_vecSaleMerch.end(); ++merchInfo ) 
	{
		//������ۿ۱�ǩ�Ͳ�����
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


//�����ȴ�����Ǵ�����˳��������������
void CSaleImpl::ReSortSaleMerch()
{
	//ֻ���л�Ա�ۿ۵Ĳ���������
	if(!IsHaveMemDiscount())
		return;
	sort((SaleMerchInfo*)m_vecSaleMerch.begin(), (SaleMerchInfo*)m_vecSaleMerch.end(), SaleMerchInfo_Promo);
}

bool CSaleImpl::IsHaveMemDiscount()
{
	return (fabs(m_fMemDiscount) > 0.001);
}


/*==============================================================*/
//��ȡ��κ�		[ Added by thinpon on Mar 6,2006 ]  �ɹ�/ʧ�� : !0 / 0
int CSaleImpl::GetShiftNO()
{
	CString strSQL;
	//unsigned int ShiftNO;2013-2-16 14:59:39 proj2013-2-16-1 �Ż� �±�����
	CString var;
	CRecordset* pRecord = NULL;
	CPosClientDB *pDB = CDataBaseManager::getConnection(true);
	if ( !pDB )
		return 0;
	
	try {
		strSQL.Format("SELECT Shift_No FROM SHIFT");
		//��ȡ���
		if ( pDB->Execute( strSQL, &pRecord ) > 0 ){
			pRecord->GetFieldValue( (short)0, var );
			m_uShiftNo=atof((LPCTSTR)var);
		}
		else{//�ް��������1��ʼ
			strSQL.Format("DELETE dbo.Shift;INSERT INTO SHIFT (Shift_NO,Shift_date) values (1,getdate())");	//aori add 2011-6-22 14:36:05 proj2011-6-22 shift ����ظ�
			if (pDB->Execute2( strSQL) == 0)//aori change 2011-3-22 16:51:36 �Ż�clientdb ��ɾ��execute2
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
		GetSalePos()->GetWriteLog()->WriteLogError("��ȡshiftno����",1);
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
		pDB->Execute2( strSQL);//aori change 2011-3-22 16:51:36 �Ż�clientdb ��ɾ��execute2
		//if ( pDB->Execute2( strSQL) > 0 ){
			//pDB->Execute2("commit");
			//return true;
		//}

		//else{//�ް��������1��ʼ
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

//������Ʒ�����ܼ���
double CSaleImpl::GetTotalCount()
{
	return GetTotalCount(&m_vecSaleMerch);
}
//������Ʒ�����ܼ���
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


//thinpon ��ȡ��ǰʱ��[����]
long CSaleImpl::GetLocalTimeSecs()
{
	time_t ltime;
	time( &ltime );
	return ltime;
}
//thinpon ��ӡ����СƱ
BOOL CSaleImpl::PrintShiftBill(int shift_no)
{
	//��ӡ�����������˳�
	
	if (!GetSalePos()->GetPrintInterface()->m_bValid) 
		return false;

	//int nMaxLen = GetSalePos()->GetParams()->m_nMaxPrintLineLen;
	int i = 0;	
//	if ( nMaxLen < 32 )
	//if ( GetSalePos()->GetParams()->m_nPaperType == 1 ) //mabh
	//	nMaxLen = 32;
	CPrintInterface  *printer = GetSalePos()->GetPrintInterface();
	//char *pt = m_szBuff;
	//��ȡpos�շ�����
	CFinDataManage m_FinDataManage;

	vectcadshift.clear();
	vectpaycadshift.clear();
	m_FinDataManage.GetChargeAmtDaily(m_stSaleDT,GetSalePos()->GetParams()->m_nPOSID,1,vectcadshift,vectpaycadshift); //mabh
	//getcadamt(shift_no);  mabh
 	printer->nSetPrintBusiness(PBID_SHIFT_BILL);
	printer->FeedPaper_Head(PBID_SHIFT_BILL);
	//��ӡ����
	//GetPrintChars(m_szBuff, nMaxLen, nMaxLen, true);
	printer->nPrintHorLine();

	GetPrintFormatAlignString("**********�����¼СƱ**********",m_szBuff,DT_CENTER);
	printer->nPrintLine( m_szBuff );

	SYSTEMTIME m_stSaleDT;
	GetLocalTimeWithSmallTime(&m_stSaleDT);
	sprintf(m_szBuff,"��̨����:%3d   ʱ��:%04d-%02d-%02d %02d:%02d",GetSalePos()->GetParams()->m_nPOSID,m_stSaleDT.wYear,m_stSaleDT.wMonth,m_stSaleDT.wDay,m_stSaleDT.wHour,m_stSaleDT.wMinute);
	printer->nPrintLine( m_szBuff );

	sprintf(m_szBuff,"���:%03d    ����Ա:%s",m_uShiftNo,GetSalePos()->GetCasher()->GetUserCode());
	printer->nPrintLine( m_szBuff );

	//��ο�ʼʱ��
	SYSTEMTIME _tm;
	GetShiftBeginTime(_tm);
	sprintf(m_szBuff,"�ϻ�ʱ��:%04d-%02d-%02d %02d:%02d",_tm.wYear,_tm.wMonth,_tm.wDay,_tm.wHour,_tm.wMinute);
	printer->nPrintLine( m_szBuff );
	//��ν���ʱ��
	::GetLocalTimeWithSmallTime(&_tm);
	sprintf(m_szBuff,"�»�ʱ��:%04d-%02d-%02d %02d:%02d",_tm.wYear,_tm.wMonth,_tm.wDay,_tm.wHour,_tm.wMinute);
	printer->nPrintLine( m_szBuff );
	//��ʼ����
	sprintf(m_szBuff,"��ʼ����:%d",GetFirstSaleID(m_uShiftNo));
	printer->nPrintLine( m_szBuff );
	//��������
	sprintf(m_szBuff,"��������:%d",GetLastSaleID(m_uShiftNo));
	printer->nPrintLine( m_szBuff );
	//��������
	sprintf(m_szBuff,"��������:%d",GetShiftTotalSale(m_uShiftNo));
	printer->nPrintLine( m_szBuff );
	//Ʒ����
	sprintf(m_szBuff,"����Ʒ����:%d",GetShiftTotalSaleItem(m_uShiftNo));
	printer->nPrintLine( m_szBuff );

	//֧����ʽ���ϼƽ��[���10��֧����ʽ]
// 	int numRec=0;
// 	CString str1("Ĩ��");
// 	CString str2("������");
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
// 	sprintf(m_szBuff,"Ӧ��:%.02f",ShiftTotal);
// 			printer->nPrintLine( m_szBuff );
//    	sprintf(m_szBuff,"ʵ��:%.02f",ShiftTotal-ShiftrealTotal);
// 			printer->nPrintLine( m_szBuff );
// 
// 
//  	//ȡ���շѻ�����
// 	double poschargeamt = 0;
// 	StyleID[0] = 0;
// 	for (i=1; i<10; i++)
// 	{
// 		StyleID[i] = i + 1;
// 	}
// 	getcadamt(shift_no,StyleID);
// 	printer->nPrintLine( "  " );
// 	sprintf(m_szBuff, "����ҵ�����룺   %11.2f",cadamt[0]+cadamt[2]);
// 	printer->nPrintLine(m_szBuff);
// 	sprintf(m_szBuff, "����ҵ���������:%11.2f",cadamt[1]);
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
// 	//����ɢ������ͳ��
// 	if (bSKSell)
// 	{
// 		StyleID[0] = 1;
// 		for (i=1; i<10; i++)
// 		{
// 			StyleID[i] = -1;
// 		}
// 		getcadamt(shift_no,StyleID);
// 		printer->nPrintLine( "  " );
// 		sprintf(m_szBuff, "ɢ�����۽�   %11.2f",(cadamt[0]+cadamt[2]));
// 		printer->nPrintLine(m_szBuff);
// 		sprintf(m_szBuff, "ɢ���˻����:    %11.2f",cadamt[1]);
// 		printer->nPrintLine(m_szBuff);
// 		poschargeamt = poschargeamt+ cadamt[0]+cadamt[2] - cadamt[1];
// 	}
// 	//����������ֵ������ͳ��
// 	if (bCZKSell)
// 	{
// 		for (i=0; i<10; i++)
// 		{
// 			StyleID[i] = 11;
// 		}
// 		getcadamt(shift_no, StyleID);
// 		printer->nPrintLine( "  " );
// 		sprintf(m_szBuff, "����Ԥ�����۽�   %2.2f",(cadamt[0]+cadamt[2]));
// 		printer->nPrintLine(m_szBuff);
// 		sprintf(m_szBuff, "����Ԥ���˻����:    %5.2f",cadamt[1]);
// 		printer->nPrintLine(m_szBuff);
// 		poschargeamt = poschargeamt+ cadamt[0]+cadamt[2] - cadamt[1];
// 	}
   
/*	getcadamt(shift_no,2);
	printer->nPrintLine( "  " );
	sprintf(m_szBuff, "�ֻ���ֵ�շ����룺   %11.2f",cadamt[0]+cadamt[2]);
	printer->nPrintLine(m_szBuff);
	sprintf(m_szBuff, "�ֻ���ֵ�շ��������:%11.2f",cadamt[1]);
	printer->nPrintLine(m_szBuff);

	poschargeamt = poschargeamt+ cadamt[0]+cadamt[2];*/

// 	printer->nPrintLine( "  " );
// 	sprintf(m_szBuff,"�ϼ�Ӧ��:%.02f",ShiftTotal+poschargeamt);
// 			printer->nPrintLine( m_szBuff );
//    	sprintf(m_szBuff,"�ϼ�ʵ��:%.02f",ShiftTotal-ShiftrealTotal+poschargeamt);

	/*printer->nPrintLine( "  " );
	sprintf(m_szBuff, "����ҵ�����룺   %11.2f",cadamt[0]+cadamt[2]);
	printer->nPrintLine(m_szBuff);
	sprintf(m_szBuff, "����ҵ���������:%11.2f",cadamt[1]);
	printer->nPrintLine(m_szBuff);

	printer->nPrintLine( "  " );
	sprintf(m_szBuff,"�ϼ�Ӧ��:%.02f",ShiftTotal+cadamt[0]);
			printer->nPrintLine( m_szBuff );
   	sprintf(m_szBuff,"�ϼ�ʵ��:%.02f",ShiftTotal-ShiftrealTotal+cadamt[0]);
	printer->nPrintLine( m_szBuff );*/
	//CString hh;hh.Format("ӡ������:%d",GetShiftTotal_STAMPCOUNT(m_uShiftNo));//�����߻������ Ӫ��������Ϣ���ĵ�����
	//printer->nPrintLine(hh );


//	for (ti=0;ti<20;ti++) free(title[ti]);
	//delete []title;
	
	//��ӡ����
	printer->nPrintHorLine();

	//for ( int j=0;j < 4;j++) {	printer->nPrintLine(" "); }  //mabh
	//for (int j=0;j<GetSalePos()->GetParams()->m_nPrintFeedNum;j++)
	//{
	//	printer->nPrintLine("   ");
	//}
	printer->FeedPaper_End(PBID_SHIFT_BILL);

	//��ֽ [dandan.wu 2016-3-7]
	printer->nEndPrint_m();
	GetSalePos()->GetWriteLog()->WriteLog("CSaleImpl::PrintShiftBill(),��ֽ",POSLOG_LEVEL1);

	vectcadshift.clear();
	vectpaycadshift.clear();
	//printer->nSetPrintBusiness(PBID_TRANS_RECEIPT);

	return true;

}

//thinpon. ���°�ο�ʼʱ��Ϊ��ǰʱ��
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
		pDB->Execute2( strSQL);//aori change 2011-3-22 16:51:36 �Ż�clientdb ��ɾ��execute2
		
	} catch( CException * e ) {
		CUniTrace::Trace(*e);
		e->Delete( );
		CDataBaseManager::closeConnection(pDB);
		return false;
	}

	CDataBaseManager::closeConnection(pDB);
	return true;		
}
//thinpon ��ȡ��ʼ���ʱ��
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
/* ��ȡ��ε��ױ����ۺ����һ������ */
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

//��ȡ������۵���
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
}//��ȡ���ӡ���������߻������ Ӫ��������Ϣ���ĵ�����
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
// �� �� ����GetShiftTotalSaleItem
//��ð��������Ʒ��
// ���������ULONG shift_no
// ���������bool
// �������ڣ�00-2-21
// �޸����ڣ�2009-4-1 ������
// ��  �ߣ�***
// ����˵�������ݿ����޸� SaleItem
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

//thinpon. ��ȡ�ֽ�֧Ʊ��֧�����ϼ�
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
//thinpon. �°���Ƿ�������(�Ƿ���Ҫ����)
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
//��ȡ��Ա������(Զ��)
long CSaleImpl::GetMemberIntergral()
{
	long ret=-1;

	//��������-1
	if (!GetSalePos()->GetParams()->m_bUseNet) return -1;
    //ϵͳ��Ա���ŷ���-9
	if (!strcmp(GetSalePos()->GetParams()->m_sSystemVIP,GetMemberCode())) return -9;

	CPosClientDB* pDB = CDataBaseManager::getConnection(true);
	if ( !pDB ) {
		CUniTrace::Trace("CSaleImpl::GetMemberIntergral");
		CUniTrace::Trace(MSG_CONNECTION_NULL, IT_ERROR);
		//����
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
				//�޴˿�
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
// �� �� ����PrintBillBootomSmall
// ������������ӡ
// ���������
// ���������void
// �������ڣ�2009-4-5 
// �޸����ڣ�
// ��  �ߣ�������
// ����˵����
//======================================================================
void CSaleImpl::PrintBillBootomSmall()
{
	//��ӡ�����������˳�
	int printBusinessID=PBID_TRANS_RECEIPT;
	if(m_ServiceImpl.HasSVMerch())
		printBusinessID=PBID_SERVICE_RECEIPT;

	if (!GetSalePos()->GetPrintInterface()->m_bValid) 
		return;
	//��¼��־
	//GetSalePos()->GetWriteLog()->WriteLog("��ʼ���ô�ӡСƱβ",4);//[TIME OUT(Second) 0.000065]
	TPrintBottom pTPrintBottom;
	TPrintBmp pTPrintBmp;

	//��ӡ����СƱ����bmp
	if (GetSalePos()->GetParams()->m_nPrintSaleBarcode == 1) {
		//��ӡbmp
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

	//��ӡ����СƱ
	if(IsEMSale() || IsEOrder())
	{
		GetSalePos()->GetPrintInterface()->nPrintHorLine(); 
		CEorderinterface* m_eorderinterface = GetSalePos()->GetEorderinterface();
		CStringArray printArray;
		m_eorderinterface->GetPrintArray(&printArray);
		GetSalePos()->GetPrintInterface()->nPrintBill(&printArray);
	}

	//�ٰ���û�д�����[dandan.wu 2016-3-15]
	//��ӡ֧Ʊ���
	//PrintBillTotalCG();

	//��ӡ��ֵ����һ��
	PrintBillTotalPrepaid();
	//��ӡ���ŵ�һ��
	PrintBillTotalMiya();

	//��¼��־
	GetSalePos()->GetWriteLog()->WriteLog("��ʼ���ô�ӡ�Ż�ȯ",4);//[TIME OUT(Second) 0.000065]
	//����ͨ���Żݽӿڸ��ʽ zenghl@nes.com.cn on 20090411
	CCouponImpl *CouponImpl=GetSalePos()->GetCouponImpl();
	if (CouponImpl!=NULL)
	{
			if (CouponImpl->m_bValid)
			{
				CouponImpl->SetSaleImpl(this);
				HANDLE hPrint = NULL;
				CString strTmp=m_PayImpl.GetAllPayment();//֧����ʽ
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
	
		if ((stricmp (m_PayImpl.GetPaymentStyle(pay->nPSID)->szInterfaceName,"ICBC.dll")) == 0 )//����СƱ�ϴ�ӡ
		{	
			//��¼��־
			GetSalePos()->GetWriteLog()->WriteLog("��ʼ���ô�ӡ����֧����",4);
			//���д�ӡ
			GetSalePos()->GetPrintInterface()->nPrintBill(&GetSalePos()->GetPaymentInterface()->ArrayBill);

		}
	
				
    }//end if for ( vector<PayInfo>  mabh*/
	/*if (stricmp (ps->szInterfaceName,"ICBC.dll") == 0) {
		
		//��¼��־
		GetSalePos()->GetWriteLog()->WriteLog("��ʼ���ô�ӡ����֧����",4);//[TIME OUT(Second) 0.000065]
		//���д�ӡ
		GetSalePos()->GetPrintInterface()->nPrintBill(&GetSalePos()->GetPaymentInterface()->ArrayBill);
	}*/
		//��¼��־
	
	//GetSalePos()->GetWriteLog()->WriteLog("��ʼ���ô�ӡ����֧����",4);//[TIME OUT(Second) 0.000065]
	//���д�ӡ
	//GetSalePos()->GetPrintInterface()->nPrintBill(&GetSalePos()->GetPaymentInterface()->ArrayBill);
	if(m_ServiceImpl.HasSVMerch())
		GetSalePos()->GetPrintInterface()->nSetPrintBusiness(PBID_SERVICE_RECEIPT);
	else
		GetSalePos()->GetPrintInterface()->nSetPrintBusiness(PBID_TRANS_RECEIPT);
	GetSalePos()->GetPrintInterface()->FeedPaper_End(printBusinessID);

	PrintBillTotalPrepaid(true);
	
	PrintBillTotalMiya(true);

	GetSalePos()->GetPrintInterface()->nEndPrint_m();
	GetSalePos()->GetWriteLog()->WriteLog("CSaleImpl::PrintBillBootomSmall(),��ֽ",POSLOG_LEVEL1);

	return;

}
//======================================================================
// �� �� ����PrintBillHeadSmall
// ������������ӡ
// ���������
// ���������void
// �������ڣ�2009-4-5 
// �޸����ڣ�
// ��  �ߣ�������
// ����˵����
//======================================================================
void CSaleImpl::PrintBillHeadSmall()
{
	if (!GetSalePos()->GetPrintInterface()->m_bValid) 
		return;//��ӡ�����������˳�
	//GetSalePos()->GetWriteLog()->WriteLog("��ʼ��ӡ��ͷ",4);////��¼��־[TIME OUT(Second) 0.000065]
	TPrintHead	pPrintHead;//char *pt = m_szBuff;//aori del
	GetPrintFormatAlignString(GetSalePos()->GetPrintInterface()->strTOP,m_szBuff,DT_CENTER);
	strcpy(pPrintHead.szMessage,m_szBuff);  //̧ͷ��Ϣ(�̵�����)

	pPrintHead.nSaleID=m_nSaleID;			//���һ�ʱ����������ˮ��

	/************************************************************************/
	/*Modified by dandan.wu 2017-08-16
	*m_stSaleDT:������ʱ���SaleItem_Temp��ʱ��
	*m_stEndSaleDT:�����±�SaleItem%s��ʱ�䣬Ӧ��Ҳ�Ǵ�ӡСƱ��ʱ��
	*/
	/************************************************************************/
// 	sprintf(m_szBuff, "����:%04d%02d%02d %02d	:%02d", 
// 	m_stSaleDT.wYear, m_stSaleDT.wMonth, m_stSaleDT.wDay, m_stSaleDT.wHour, m_stSaleDT.wMinute);
// 	pPrintHead.stSaleDT=m_stSaleDT;			//����/ʱ��
	sprintf(m_szBuff, "����:%04d%02d%02d %02d	:%02d", 
		m_stEndSaleDT.wYear, m_stEndSaleDT.wMonth, m_stEndSaleDT.wDay, m_stEndSaleDT.wHour, m_stEndSaleDT.wMinute);
	pPrintHead.stSaleDT=m_stEndSaleDT;			
	
	pPrintHead.nUserID=atoi(GetSalePos()->GetCasher()->GetUserCode());

	//��Ա��
	memcpy(pPrintHead.szMemberCode,GetMember().szMemberCode,strlen(GetMember().szMemberCode));

	//��Ա����
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
// �� �� ����PrintBillItemSmall
// ������������ӡ����豸
// ���������
// ���������void
// �������ڣ�2009-4-5 
// �޸����ڣ�
// ��  �ߣ�������
// ����˵����
//======================================================================
// void CSaleImpl::PrintBillItemSmall(CString szPLU, CString szSimpleName, double qty, double price, double amnt)
// {
// 	if (!GetSalePos()->GetPrintInterface()->m_bValid) return;//��ӡ�����������˳�
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
		return; //��ӡ�����������˳�
	SaleMerchInfo pSaleItem=*pmerchinfo; 
	//mabh �ж�13,18�� ����ӡ����
	if (pSaleItem.nManagementStyle == 2 || pSaleItem.bFixedPriceAndQuantity) 
		pSaleItem.bPriceInBarcode = true;
 
	pSaleItem.fActualPrice  = price;
	pSaleItem.fSaleQuantity = qty;

	//�ۿۺ󵥼� [dandan.wu 2017-4-12]
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
	SaleMerchInfo pSaleItem=*pmerchinfo;//��ӡ�����������˳�
	pSaleItem.fActualPrice  = price;
	pSaleItem.fSaleQuantity = qty;
	GetSalePos()->GetPrintInterface()->nPrintSalesItem_m(&pSaleItem,PrintBusinessID);//mabh
	return;
}//aori end 2011-12-30 14:42:38 proj 2011-12-22-1 combine 2.75 with 2.71 saleimpl
*/
//======================================================================
// �� �� ����PrintBillTotalSmall
// ������������ӡ
// ���������
// ���������void
// �������ڣ�2009-4-5 
// �޸����ڣ�//zenghl 20090520 ����Զ�̻��ִ�ӡ
// ��  �ߣ�������
// ����˵����
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
		//���� ��ӡ
		for ( vector<TPromotion>::iterator pPromotion = vPromotion.begin(); pPromotion != vPromotion.end(); ++pPromotion)
		{
			GetSalePos()->GetPrintInterface()->nPrintPromotion(pPromotion);
			printLine=true;
		}

		//���� ��ӡ(�ؿ�СƱʱ��)
		if ( vPromotion.size() == 0 ) 
		{ 
			vector<TPromotion> vreloadPromotion=GetSalePos()->GetPromotionInterface()->m_vreloadPromotion;
			for ( vector<TPromotion>::iterator preloadPromotion = vreloadPromotion.begin(); preloadPromotion != vreloadPromotion.end(); ++preloadPromotion)
			{
				GetSalePos()->GetPrintInterface()->nPrintPromotion(preloadPromotion);//��ӡ����
				printLine=true;
			}
			GetSalePos()->GetPromotionInterface()->m_vreloadPromotion.clear();  
		}
			//����
//		if(printLine)
//			GetSalePos()->GetPrintInterface()->nPrintHorLine(); 

		//��ӡ��Ա�ۿ��ܽ��ͽ�ʡ�ܽ��
		printBillSumDiscount();

		//��ӡ�ϼ�
		TPrintTotal_SUM	pPrintTotale_sum;
		fTotal=GetTotalAmt_beforePromotion();
		//ReducePrecision2(fTotal,GetSalePos()->GetParams()->m_nPayPrecision);//��������ʱ Ӧ����� ��� ���� GetTotalAmt_beforePromotion
		if ( strlen(m_MemberImpl.m_Member.szMemberCode) > 1 )
		{
			if (GetSalePos()->GetParams()->m_bWallet_Wipe_Zero)   //ʹ����Ǯ���Ƿ�Ĩ��  0:��Ĩ�㣬 1��Ĩ��
				ReducePrecision2(fTotal,GetSalePos()->GetParams()->m_nPayPrecision);//2:���� 1://���
		}
		else
			ReducePrecision2(fTotal,GetSalePos()->GetParams()->m_nPayPrecision);
	
		strcpy(pPrintTotale_sum.szMemberCode,GetMember().szMemberCode);

		pPrintTotale_sum.nMemberScore=GetMember().dTotIntegral;
		if (strcmp(GetSalePos()->GetParams()->m_sSystemVIP,GetMember().szMemberCode)==0)
			pPrintTotale_sum.isSystem=1;
		else
			pPrintTotale_sum.isSystem=0;
		pPrintTotale_sum.nHotScore=GetSalePos()->GetPromotionInterface()->m_nHotReducedScore;//���ٻ���
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
		//��ӡ���������Żݽ��
		if(amt_discount>0)
		{
			//GetSalePos()->GetPrintInterface()->nPrintHorLine(); 
			sprintf(m_szBuff,"���������Żݽ��: -%.2f", amt_discount);
			GetSalePos()->GetPrintInterface()->nPrintLine(m_szBuff);
		}
		//��ӡ�ϼ�
		TPrintTotal_SUM	pPrintTotale_sum;
		double fTotal=amt_afterProm;
		//ReducePrecision2(fTotal,GetSalePos()->GetParams()->m_nPayPrecision);//��������ʱ Ӧ����� ��� ���� GetTotalAmt_beforePromotion
		if ( strlen(m_MemberImpl.m_Member.szMemberCode) > 1 )
		{
			if (GetSalePos()->GetParams()->m_bWallet_Wipe_Zero)   //ʹ����Ǯ���Ƿ�Ĩ��  0:��Ĩ�㣬 1��Ĩ��
				ReducePrecision2(fTotal,GetSalePos()->GetParams()->m_nPayPrecision);//2:���� 1://���
		}
		else
			ReducePrecision2(fTotal,GetSalePos()->GetParams()->m_nPayPrecision);
		strcpy(pPrintTotale_sum.szMemberCode,GetMember().szMemberCode);
		pPrintTotale_sum.nMemberScore=GetMember().dTotIntegral;
		if (strcmp(GetSalePos()->GetParams()->m_sSystemVIP,GetMember().szMemberCode)==0)
			pPrintTotale_sum.isSystem=1;
		else
			pPrintTotale_sum.isSystem=0;
		pPrintTotale_sum.nHotScore=GetSalePos()->GetPromotionInterface()->m_nHotReducedScore;//���ٻ���
		pPrintTotale_sum.nTotalQty=GetTotalCount(pVecSaleMerch);
		pPrintTotale_sum.nTotalAmt=fTotal;
		pPrintTotale_sum.fRebateAccAmt=GetMember().fRebateAccAmt;
		pPrintTotale_sum.fWalletAccAmt=0;
		strcpy(pPrintTotale_sum.szRebateAccBeginDate,GetMember().szRebateAccBeginDate);
		strcpy(pPrintTotale_sum.szRebateAccEndDate,GetMember().szRebateAccEndDate);
		GetSalePos()->GetPrintInterface()->nPrintTotal_Sum(&pPrintTotale_sum); 
	}



//<------------------���m_bPoolClothesMerchΪtrue��ӡ��Ӫ������Ʒ��СƱ
	const TPaymentStyle *ps = NULL;
	if(!GetSalePos()->GetParams()->m_bPoolClothesMerch)  //����СƱ����ӡ��������
	{	
		double FaPiaoTotal=0.0;//aori add 2011-2-9 13:40:07 need ��Ʊ����ӡ

		//printLine=false; 

		for ( vector<PayInfo>::const_iterator pay = m_PayImpl.m_vecPay.begin(); pay != m_PayImpl.m_vecPay.end(); ++pay) 
		{
 			TPaymentItem	pPaymentItem;
	
			ps = m_PayImpl.GetPaymentStyle(pay->nPSID);
			if ( NULL != ps && ps->bPrintOnVoucher)//����СƱ�ϴ�ӡ
			{	
				//�ӱ��ж�ȡ�Ƿ��ӡ�����索ֵ����Ҫ��ӡ��� [dandan.wu 2016-3-17]
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
							//֧����������
							strcpy(pPaymentItem.szDescription,ps->szDescription);
						
							//֧�����
							pPaymentItem.fPaymentAmt = payInfoItem->fPaymentAmt;
							
							//���
							if (pPaymentItem.bPrintRemainAmt)
								pPaymentItem.fRemainAmt = payInfoItem->fRemainAmt;

							//����
							strcpy(pPaymentItem.szPaymentNum,payInfoItem->szPaymentNum);

							//��Ϊ���ڸ����Ҫ��ӡ����ID���������������� [dandan.wu 2016-3-28]
							if ( payInfoItem->nPSID == ::GetSalePos()->GetParams()->m_nInstallmentPSID )
							{
								pPaymentItem.nLoanID = payInfoItem->nLoanID;
								pPaymentItem.nLoanPeriod = payInfoItem->nLoanPeriod;
								if ( strlen(payInfoItem->szLoanType) == 0 )
								{
									CUniTrace::Trace("���ۣ���������Ϊ��");
								}
								else
								{
									strcpy(pPaymentItem.szLoanType,payInfoItem->szLoanType);
								}

								if ( strlen(payInfoItem->szLoanID) == 0 )
								{
									CUniTrace::Trace("���ۣ�����ID����Ϊ��");
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
									CUniTrace::Trace("���ۣ�������ϢΪ��");
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
									CUniTrace::Trace("���ۣ�������ϢΪ��");
								}
								else
								{
									strcpy(pPaymentItem.szExtraInfo,payInfoItem->szExtraInfo);
								}
							}

							//aori add ���п���ӡ���ε�����5������12λ
							HideBankCardNum(pay, pPaymentItem);
							
							if (pPaymentItem.fPaymentAmt > 0 )
		    						GetSalePos()->GetPrintInterface()->nPrintPayment(&pPaymentItem);//��ӡ֧��							
						}
					}
				}
				else
				{
					//���
					strcpy(pPaymentItem.szDescription,ps->szDescription);
					pPaymentItem.fPaymentAmt = pay->fPayAmt;
					
					if (!ps->bNeedReadCard) //��IC����paynum
							strcpy(pPaymentItem.szPaymentNum,pay->szPayNum);

					if (pPaymentItem.fPaymentAmt > 0 )
		    				GetSalePos()->GetPrintInterface()->nPrintPayment(&pPaymentItem);//��ӡ֧��
				}

				if ( ps->bForChange )
					forchange_amt -= pay->fPayAmt;
				
				if ( ps->bForInvoiceAmt )//aori add 2011-2-9 13:40:07 need ��Ʊ����ӡ
					FaPiaoTotal += pay->fPayAmt;
				 		
		    //printLine=true;
			}	
		}
		//����
		//if(printLine)
		//	GetSalePos()->GetPrintInterface()->nPrintHorLine(); 

		TPrintTotal_change pPrintTotale_change;
		double dTemp = m_PayImpl.GetChange();
		pPrintTotale_change.nPayAmt=m_PayImpl.GetPayAmt()+forchange_amt;
		pPrintTotale_change.nChange=dTemp;
		GetSalePos()->GetPrintInterface()->nPrintTotal_Change(&pPrintTotale_change);//��ӡ����
		
		//֧�����ۿ۽��
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
				//����֧����ȡ��ϸ
				m_PayImpl.GetPayInfoItemVect(alipay->nPSID, &vecFindPayItem);

				for ( vector<PayInfoItem>::iterator payInfoItem = vecFindPayItem.begin(); payInfoItem != vecFindPayItem.end(); ++payInfoItem)
				{				

					if (payInfoItem->fRemainAmt > 0 )
					{
						alipay_Discount=alipay_Discount+payInfoItem->fRemainAmt;
						m_szDescription.Format("%s",ps->szDescription);
					}
				}
				//�ϼ����ֵ��Ϊ�ۿ۽��
		    //printLine=true;
			}	
		}
		//alipay_Discount=3.0; //test
		if (alipay_Discount > 0)
		{
			sprintf(m_szBuff, "%s�ۿ۽��:%13.2f ",m_szDescription,-alipay_Discount);
			GetSalePos()->GetPrintInterface()->nPrintLine(m_szBuff);
			//sprintf(m_szBuff, "ʵ��֧�����:%15.2f ",m_PayImpl.GetPayAmt()+forchange_amt-dTemp-alipay_Discount);
			//GetSalePos()->GetPrintInterface()->nPrintLine(m_szBuff);
			//����
			//GetSalePos()->GetPrintInterface()->nPrintHorLine(); 
		}

		//��ӡ�����ֵ
		ps = NULL;
		for ( vector<PayInfo>::const_iterator pay2 = m_PayImpl.m_vecPay.begin(); pay2 != m_PayImpl.m_vecPay.end(); ++pay2) 
		{
			TPaymentItem		pPaymentItem;
			ps = m_PayImpl.GetPaymentStyle(pay2->nPSID);
			if ( NULL != ps && ps->bPrintOnVoucher && ps->bForChange)
			{
				//����СƱ�ϴ�ӡ
				//�ȴ�ӡpayment
				strcpy(pPaymentItem.szDescription,ps->szDescription);
				pPaymentItem.fPaymentAmt=-pay2->fPayAmt;
				pPaymentItem.bPrintRemainAmt = ps->bPrintRemainAmt;
				if (!ps->bNeedReadCard) //��IC����paynum
					strcpy(pPaymentItem.szPaymentNum,pay2->szPayNum);
				HideBankCardNum( pay2,pPaymentItem);//aori add ���п���ӡ���ε�����5������8λ
				GetSalePos()->GetPrintInterface()->nPrintPayment(&pPaymentItem);//��ӡ֧��
			}	
		}
		//GetPrintChars(m_szBuff, nMaxLen,nMaxLen, true);
		//GetSalePos()->GetPrintInterface()->nPrintLine_m( m_szBuff ,printBusinessID);//����
		//GetPrintChars(m_szBuff, nMaxLen,nMaxLen, true);GetSalePos()->GetPrintInterface()->nPrintLine( m_szBuff );//����//aori 2011-12-30 14:42:38 proj 2011-12-22-1 combine 2.75 with 2.71 saleimpl
		
		//��ӡ��ϸpaymentitem IC��
		printLine=false;
		ps = NULL;
		const TPaymentStyle *psItem = NULL;
		for ( vector<PayInfo>::const_iterator pay1 = m_PayImpl.m_vecPay.begin(); pay1 != m_PayImpl.m_vecPay.end(); ++pay1) 
		{
			TPaymentItem		pPaymentItem;
			vector<PayInfoItem> vecFindPayItem;

			ps = m_PayImpl.GetPaymentStyle(pay1->nPSID);

			//����֧����ȡ��ϸ
			m_PayImpl.GetPayInfoItemVect(pay1->nPSID, &vecFindPayItem);
			if ( NULL != ps && ps->bNeedReadCard )
			{	
				//bNeedReadCard
				CString extp,strBuf;
				int prtline = -1;
				//��ȡ��ӡ����
				GetSalePos()->GetWriteLog()->WriteLog("��ȡ��ӡ����",3,POSLOG_LEVEL3); 
				extp.Format("%s",ps->szExtParams);
				strBuf.Format("֧������������%s",extp);
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
					//֧���������
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
						pPaymentItem.stSaleDT=payInfoItem->stEffectDT;//��Ч��(ic��ʹ��)
					}
					if ( ps->bPrintOnVoucher )
					{
						//����СƱ�ϴ�ӡ
						HideBankCardNum( pay1,pPaymentItem);//aori add ���п���ӡ���ε�����5������8λ
						i=i+1;
						if ( prtline < 0) 
						{
							strBuf.Format("�����ƴ�ӡ֧������������%d",i);
							GetSalePos()->GetWriteLog()->WriteLog(strBuf,3,POSLOG_LEVEL3); 
							GetSalePos()->GetPrintInterface()->nPrintPayment(&pPaymentItem);//��ӡ֧��
						}
						else//�д�ӡ����
						{
							if (  i < prtline ) 
							{
								strBuf.Format("��ӡ֧������������%d",i);
								GetSalePos()->GetWriteLog()->WriteLog(strBuf,3,POSLOG_LEVEL3); 
								GetSalePos()->GetPrintInterface()->nPrintPayment(&pPaymentItem);//��ӡ֧��
							}
							else 
							{ 
								if ( i == prtline ) 
								{
									strBuf.Format("��ӡ֧������������%d",i);
									GetSalePos()->GetWriteLog()->WriteLog(strBuf,3,POSLOG_LEVEL3); 
									GetSalePos()->GetPrintInterface()->nPrintPayment(&pPaymentItem);//��ӡ֧��
								}
								else
								{
									strBuf.Format("�����ӡ֧������������%d",i);
									GetSalePos()->GetWriteLog()->WriteLog(strBuf,3,POSLOG_LEVEL3); 

								}
							}
						}//�д�ӡ����
					}//����СƱ�ϴ�ӡ
					printLine=true;
				}//for
				if (prtline > 0 && i > prtline  )
				{
					strBuf.Format("��%d��֧������ʡ�Դ�ӡ!",(i-prtline));
					GetSalePos()->GetPrintInterface()->nPrintLine(strBuf);
				}

			}//bNeedReadCard				
		}
		//����
		if(printLine)
			GetSalePos()->GetPrintInterface()->nPrintHorLine(); 
		//<--#������ýӿ�#
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
		//����
		if(printLine)
			GetSalePos()->GetPrintInterface()->nPrintHorLine(); 
		//��ӡ��Ա��Ϣ
		fTotal=GetTotalAmt();
		//ReducePrecision2(fTotal,GetSalePos()->GetParams()->m_nPayPrecision);
		if ( strlen(m_MemberImpl.m_Member.szMemberCode) > 1 )
		{
			if (GetSalePos()->GetParams()->m_bWallet_Wipe_Zero)   //ʹ����Ǯ���Ƿ�Ĩ��  0:��Ĩ�㣬 1��Ĩ��
				ReducePrecision2(fTotal,GetSalePos()->GetParams()->m_nPayPrecision);//2:���� 1://���
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
		pPrintTotale_sum.nHotScore=GetSalePos()->GetPromotionInterface()->m_nHotReducedScore;//���ٻ���
		pPrintTotale_sum.nTotalQty=GetTotalCount(pVecSaleMerch);
		pPrintTotale_sum.fRebateAccAmt=GetMember().fRebateAccAmt;
		pPrintTotale_sum.fWalletAccAmt=0;
		strcpy(pPrintTotale_sum.szRebateAccBeginDate,GetMember().szRebateAccBeginDate);
		strcpy(pPrintTotale_sum.szRebateAccEndDate,GetMember().szRebateAccEndDate);
		//if (GetSalePos()->GetParams()->m_nUseRemoteMemScore==1||GetSalePos()->GetParams()->m_nUseRemoteMemScore==2
		//	||GetSalePos()->GetParams()->m_nUseRemoteMemScore==3 ||GetSalePos()->GetParams()->m_nUseRemoteMemScore==4){//zenghl 20090520
		//	pPrintTotale_sum.nMemberScore=GetSalePos()->GetPosMemberInterface()->m_fMemberRealtimeScore;//ȡԶ�̻���
		//}
		//ʵʱ����
		GetSalePos()->GetWriteLog()->WriteLog("��ӡ����",3);//��־
		if (GetSalePos()->GetParams()->m_bRealtimeJF == 1 && strlen(m_MemberImpl.m_Member.szMemberCode) > 1)
		{
			CString strbuf;

			//RealtimeJF  *m_RealtimeJF = GetSalePos()->GetRealtimeJF() ;

			pPrintTotale_sum.bScoreRealtime=true;
			pPrintTotale_sum.bScoreSuccess=m_MemberImpl.m_RealtimeScoreSuccess;
			//strbuf.Format("bScoreSuccess:%d",m_RealtimeJF->rtn_flag);
			//GetSalePos()->GetWriteLog()->WriteLog(strbuf,3);//��־
			pPrintTotale_sum.nAddScore=m_MemberImpl.m_dMemIntegral;
			//strbuf.Format("nAddScore:%d",m_RealtimeJF->rtn_jf);
			//GetSalePos()->GetWriteLog()->WriteLog(strbuf,3);//��־
			//GetSalePos()->GetRealtimeJF() ;
		}
		//pPrintTotale_sum.fMemXFSScore = m_MemXFSScore;//��Ա�����ͻ���
		//�мѻ�Ա��ӡ
		if(GetSalePos()->GetParams()->m_nUseRemoteMemScore == 5)
		{
			pPrintTotale_sum.isSystem = 2;
			GetSalePos()->GetPrintInterface()->nPrintMemberInfo(&pPrintTotale_sum);//��ӡ��Ա��Ϣ
			//��ӡ�мѻ�Ա�����
			CString strBuf;
			if (strlen(m_MemberImpl.m_Member.szVIPCardNO) > 1)
			{
				strBuf.Format("%s:%s","��Ա�����", m_MemberImpl.m_Member.szVIPCardNO);
				GetSalePos()->GetPrintInterface()->nPrintLine(strBuf);
			}
			//��ӡ�мѻ�Ա����
			if (strlen(m_MemberImpl.m_Member.szMemberName) > 1)
			{
				strBuf.Format("%s", m_MemberImpl.m_Member.szMemberName);
				GetSalePos()->GetPrintInterface()->nPrintLine(strBuf);
			}
		}
		else
		{
			GetSalePos()->GetPrintInterface()->nPrintMemberInfo(&pPrintTotale_sum);//��ӡ��Ա��Ϣ
		}
		//��ӡ��Ա��������Ϣ
		if (GetSalePos()->GetParams()->m_nGoldcardtype > 0 && strlen(m_MemberImpl.m_Member.szMemberCode) > 1)//���Դ�ӡ
		{
			CString strBuf;
			if (GetSalePos()->GetParams()->m_nGoldcardtype == atoi(GetMember().szCardTypeCode))	 //�𿨴�ӡ
			{
				strBuf.Format("%s", GetSalePos()->GetParams()->m_Goldcardprtmsg);
			}
			else	//������ӡ
			{
				strBuf.Format("%s", GetSalePos()->GetParams()->m_Silvercardprtmsg);
			}
			GetSalePos()->GetPrintInterface()->nPrintLine(strBuf);

		}
		
		//��ӡ��Ա�����ͻ���
		if (GetSalePos()->GetParams()->m_bMemberXFSScore && strlen(m_MemberImpl.m_Member.szMemberCode) > 1)
		{
			CString strBuf;
			strBuf.Format("��Ա���������ͻ���:%.2f", m_MemberImpl.m_dMemXFSScore);
			GetSalePos()->GetPrintInterface()->nPrintLine(strBuf);
			GetSalePos()->GetPrintInterface()->nPrintHorLine();
		}
		//��ӡ��Ա�н���Ϣ
		if (GetSalePos()->GetParams()->m_bMemberLottery && strlen(m_MemberImpl.m_Member.szMemberCode) > 1)
		{
			if (m_MemberImpl.m_LotteryGrade)
			{
				CString strMemberLottery;
				strMemberLottery.Format("�н��ȼ�:%d",m_MemberImpl.m_LotteryGrade);
				GetSalePos()->GetPrintInterface()->nPrintLine(strMemberLottery);
				GetSalePos()->GetPrintInterface()->nPrintHorLine(); 
			}
			else
			{
				CString strMemberLottery;
				strMemberLottery.Format("�н��ȼ�:");
				GetSalePos()->GetPrintInterface()->nPrintLine(strMemberLottery);
				GetSalePos()->GetPrintInterface()->nPrintHorLine(); 
			}
		}
		//��ӡ����֧���룬�͵��̻�Ա��
		if (IsEOrder())
		{
			if (m_EorderID)
			{
				CString strEorderID;
				strEorderID.Format("���̶�����:%s",m_EorderID);
				GetSalePos()->GetPrintInterface()->nPrintLine(strEorderID);
			}
			/*
			if (GetSalePos()->m_params.m_EOrderMemberCode)
			{
					CString strEOrderMemberCode;
					strEOrderMemberCode.Format("���̶�����:%s",GetSalePos()->m_params.m_EOrderMemberCode);
					GetSalePos()->GetPrintInterface()->nPrintLine(strEOrderMemberCode);
			}*/
		}
		
		if (IsEMSale())
		{
			/*
			if (m_EMemberID)
			{
				CString strEMemberID;
				strEMemberID.Format("���̻�Ա��:%s",m_EMemberID);
				GetSalePos()->GetPrintInterface()->nPrintLine(strEMemberID);
			}*/
			if (m_EorderID)
			{
				CString strEorderID;
				strEorderID.Format("���̶�����:%s",m_EorderID);
				GetSalePos()->GetPrintInterface()->nPrintLine(strEorderID);
			}
		}
		//aori add 2011-2-9 13:40:07 need ��Ʊ����ӡ Begin
		//double dTemp = m_PayImpl.GetChange();
		FaPiaoTotal-=dTemp;//aori add 2011-2-9 13:40:07 bug:��Ʊ��Ϊ֧����  2011-2-18 11:27:57
		//FaPiaoTotal=ReducePrecision(FaPiaoTotal);
		//ReducePrecision2(FaPiaoTotal,GetSalePos()->GetParams()->m_nPayPrecision);
		if(GetSalePos()->GetParams()->m_bPrintInvoiceAmt)
		{
			//AfxMessageBox("��ӡ��Ʊ���");
			CString Num[]={"��","Ҽ","��","��","��","��","½","��","��","��"};
			CString Wei[]={"��","��","��","Ԫ","ʰ","��","Ǫ","��","��"};
			double tmptotal=(FaPiaoTotal-alipay_Discount)*100;
			CString DaXie,pre;
			pre.Format("��");
			DaXie.Format("��");
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
						DaXie="Ԫ"+pre;
						bneedzero=FALSE;
					};
				}
				else
				{
					bhasNUM=true;
					bneedzero?(DaXie=Num[i]+Wei[1+j]+"��"+pre,bneedzero=false):
						  DaXie=Num[i]+Wei[1+j]+pre;			
				}
				pre=DaXie;
			}
			CString printfapiao;printfapiao.Format("��Ʊ���Сд:%.2fԪ \n��Ʊ����д:%s",FaPiaoTotal-alipay_Discount,DaXie);
			//AfxMessageBox(printfapiao);
		//	GetSalePos()->GetPrintInterface()->nPrintLine( printfapiao );//aori del 2011-12-30 14:42:38 proj 2011-12-22-1 combine 2.75 with 2.71 saleimpl
			GetSalePos()->GetPrintInterface()->nPrintLine( printfapiao);//	2011-12-30 14:42:38 proj 2011-12-22-1 combine 2.75 with 2.71 saleimpl
			GetSalePos()->GetPrintInterface()->nPrintHorLine(); 
		}
	//aori add 2011-2-9 13:40:07 need ��Ʊ����ӡ end
	}
	return;
}


void CSaleImpl::PrintBillTotalCG()
{	//��ӡ֧Ʊ���
	if(!GetSalePos()->GetPrintInterface()->m_bValid)
		return;
	if(!GetSalePos()->GetPrintInterface()->GetPOSPrintConfig(PBID_CHECK_STUB).Enable) 
		return;
	//GetSalePos()->GetWriteLog()->WriteLog("��ʼ��ӡ֧Ʊ���",4);//[TIME OUT(Second) 0.000065]
	bool bp=false;
	bool bNeedSign = false;
	for ( vector<PayInfo>::const_iterator pay_tmp = m_PayImpl.m_vecPay.begin(); pay_tmp != m_PayImpl.m_vecPay.end(); ++pay_tmp) 
	{
		vector<PayInfoItem> vecFindPayItem;
		m_PayImpl.GetPayInfoItemVect(pay_tmp->nPSID, &vecFindPayItem);	
/*		if (m_PayImpl.GetPaymentStyle(pay_tmp->nPSID)->bNeedPayNO)
		{
			bp=true;
			break;//����޿ɴ�ӡ�ģ�����
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
		nMaxLen = 32;	//��ӡ����
	GetPrintChars(m_szBuff, nMaxLen, nMaxLen, true);
	printer->nPrintLine( m_szBuff );
	//��ӡ̧ͷ	
	sprintf(m_szBuff, "����:%04d%02d%02d %02d:%02d����Ա:%s", 
		m_stSaleDT.wYear, m_stSaleDT.wMonth, m_stSaleDT.wDay, m_stSaleDT.wHour, m_stSaleDT.wMinute, 
		GetSalePos()->GetCasher()->GetUserCode());
	printer->nPrintLine( m_szBuff );
	sprintf(m_szBuff, "����:%03d ����:%08d", 
			GetSalePos()->GetParams()->m_nPOSID,m_nSaleID);
	printer->nPrintLine( m_szBuff );


	//β//�ܽ����ܼ���
	double fTotal=GetTotalAmt_beforePromotion();//aori change 2012-2-9 10:25:40 proj 2.79 ���Է�������������ʱ Ӧ����� ��� ���� GetTotalAmt_beforePromotion
	// ReducePrecision2(fTotal,GetSalePos()->GetParams()->m_nPayPrecision);
	//��Ǯ�����Ա��Ĩ�����
	if ( strlen(m_MemberImpl.m_Member.szMemberCode) > 1 )
	{
		if (GetSalePos()->GetParams()->m_bWallet_Wipe_Zero)   //ʹ����Ǯ���Ƿ�Ĩ��  0:��Ĩ�㣬 1��Ĩ��
		{
			ReducePrecision2(fTotal,GetSalePos()->GetParams()->m_nPayPrecision);//2:���� 1://���
		}
	
	}
	else
	{
		ReducePrecision2(fTotal,GetSalePos()->GetParams()->m_nPayPrecision);
	}


	sprintf(m_szBuff,"�������:%.0f  Ӧ�����:%.2f",GetTotalCount(),fTotal);
	printer->nPrintLine( m_szBuff );

	const TPaymentStyle *ps = NULL;

	//��ӡ֧����ʽ�����
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

		//thinpon. ��ӡ֧������
		if (NULL != ps && ps->bNeedPayNO )
		{
			sprintf(m_szBuff,"����:%s",pay->szPayNum);//aori yinhang???
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
				sprintf(m_szBuff,"���:%.2f",payInfoItem->fRemainAmt);
				printer->nPrintLine(m_szBuff);
			}
		}
	}

	//��ӡ�����
	double dTemp = m_PayImpl.GetChange();
	ReducePrecision(dTemp);
	sprintf(m_szBuff, "ʵ�ս��:%9.2f ����:%5.2f",m_PayImpl.GetPayAmt(),dTemp);
	//GetPrintFormatLines("����", m_szBuff + 200, m_szBuff, nMaxLen);
	printer->nPrintLine(m_szBuff);

	printer->nPrintLine(" ");
	printer->nPrintLine(" ");
	printer->nPrintLine(" ");
	//printer->nSetPrintBusiness(PBID_TRANS_RECEIPT);
	if (bNeedSign)
	{
		sprintf(m_szBuff, "%s%s", "��Աǩ��:","....................");
		printer->nPrintLine(m_szBuff);
		printer->nPrintLine(" ");
		sprintf(m_szBuff, "%s%s", "�˿�ǩ��:","....................");
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
			
			//��ӡ֧������
			if ( NULL != ps && ps->bNeedPayNO)
			{
				sprintf(m_szBuff,"����:%s",pay->szPayNum);
				printer->nPrintLine(m_szBuff);
			}
			if (vecFindPayItem.size() > 0)
			{
				vector<PayInfoItem>::iterator payInfoItem = vecFindPayItem.begin(); 
				sprintf(m_szBuff,"���:%.2f",payInfoItem->fRemainAmt);
				printer->nPrintLine(m_szBuff);
			}
		}
		printer->nPrintLine(" ");
		sprintf(m_szBuff, "%s%s", "��Աǩ��:","....................");
		printer->nPrintLine(m_szBuff);
		printer->nPrintLine(" ");
		sprintf(m_szBuff, "%s%s", "�˿�ǩ��:","....................");
		printer->nPrintLine(m_szBuff);
		printer->nPrintLine(" ");
		printer->nPrintLine(" ");
	}
}


//======================================================================
// �� �� ����PrintBillWHole
// ������������ӡ
// ���������
// ���������void
// �������ڣ�2009-4-6 
// �޸����ڣ�
// ��  �ߣ�������
// ����˵����//
//======================================================================
void CSaleImpl::PrintBillWHole(ePrintPart ePart)
{
	//��ӡ�����������˳�
	if (!GetSalePos()->GetPrintInterface()->m_bValid) 
	{
		GetSalePos()->GetWriteLog()->WriteLog("END PrintBillWhole PrintInterface::m_bValid��ֹ��ӡ",4,POSLOG_LEVEL1); 
		return;
	} 

	if(!GetSalePos()->GetPrintInterface()->GetPOSPrintConfig(PBID_TRANS_RECEIPT).Enable)
	{
		GetSalePos()->GetWriteLog()->WriteLog("END PrintBillWhole PrintInterface::��ӡ����Ϊ����ӡ����СƱ",4,POSLOG_LEVEL1); 
		return;
	}

	if(m_ServiceImpl.HasSVMerch())
		GetSalePos()->GetPrintInterface()->nSetPrintBusiness(PBID_SERVICE_RECEIPT);
	else
		GetSalePos()->GetPrintInterface()->nSetPrintBusiness(PBID_TRANS_RECEIPT);

	//��ӡ����СƱ
 	if (GetSalePos()->GetParams()->m_bPoolClothesMerch)
 	{
 		//����СƱ
 		GetSalePos()->GetPrintInterface()->nPrintLine("---------- �տ�Ա�� -----------");
 		
 		/************************************************************************/
 		/* Modified by dandan.wu 2015-12-29*/
 		/*��������Ԥ��ӡСƱ��ť����ֻ���ӡ���沿�֣�����ȫ��ӡ*/
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
			CUniTrace::Trace(_T("��Ч�Ĵ�ӡ��"));
		}
 		/************************************************************************/
 
 		//����Ӫ����СƱ
 		map<CString, double> mapPoolClothesMerch;
 		GetPoolClothesMerch(&m_vecSaleMerch,&mapPoolClothesMerch);
 		int count = 1;
 		map<CString, double>::iterator itr; 
 		for (itr = mapPoolClothesMerch.begin(); itr != mapPoolClothesMerch.end(); itr++, count++)
 		{	
 			GetSalePos()->GetPrintInterface()->nPrintLine(" ");
 			sprintf(m_szBuff,"---ӪҵԱ��:(�� %d ��) �� %d ��---",mapPoolClothesMerch.size(), count);
 			GetSalePos()->GetPrintInterface()->nPrintLine(m_szBuff);
 			CString poolSKU=itr->first;
 			sprintf(m_szBuff," ��Ӫ����: %s ",poolSKU);
 			GetSalePos()->GetPrintInterface()->nPrintLine(m_szBuff);
 			GetSalePos()->GetPrintInterface()->nPrintHorLine();
 
 			//��ȡ��Ӫ�����Ӧ��Ʒ
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
 			//��ӡ
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
	    /*��������Ԥ��ӡСƱ��ť����ֻ���ӡ���沿�֣�����ȫ��ӡ*/
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
			CUniTrace::Trace(_T("��Ч�Ĵ�ӡ��"));
		}
		/************************************************************************/
	}
}
// 
//��ӡ����СƱ
void CSaleImpl::PrintBillWhole_ONE()
{
	GetSalePos()->GetWriteLog()->WriteLog("BEGIN PrintBillWhole_ONE",4,POSLOG_LEVEL1); 

	GetSalePos()->GetWriteLog()->WriteLog("��ʼ��ӡСƱ",2);////��¼��־---------------------
	
	//��ӡСƱͷ&��Ʒ�У������д�ӡ��
	if (GetSalePos()->m_params.m_nPrinterType==1)
	{
		SortSaleMerch();
		PrintBillHeadSmall();//СƱͷ
		m_bReprint?PrintBillLine(g_STRREPRINT):1;//��Ʒ��//int nMerchNum=0;//aori del
//		for (vector<SaleMerchInfo>::iterator merchInfo = m_vecSaleMerch.begin(); merchInfo != m_vecSaleMerch.end(); ++merchInfo) 
//		{
//			if (merchInfo->nMerchState != RS_CANCEL)
//			{
//				double fsalequantity=0.0f,factualprice=0.0f;
//				merchInfo->OutPut_Quantity_Price(fsalequantity,factualprice);
//			 	PrintBillItemSmall(merchInfo,fsalequantity,factualprice); //aori optimize 2011-10-13 14:53:19 ���д�ӡreplace//printbillitemsmall(merchinfo->szplu,szname,fsalequantity,factualprice,merchinfo->fsaleamt); //cstring szname;szname.format("%s%s",merchinfo->npromoid>0?"(��)":"",merchinfo->szsimplename);//bug��2011-2-24 10:52:36������ӡ��promoid ������salepromoid�жϡ�
//			 }
//		}

		//��ӡ�ֻ���Ʒ��ϸ
		printBillItemInStock(m_vecSaleMerch);
		
		//��ӡ������ϸ
		printBillItemInOrder(m_vecSaleMerch);
	}

	m_bReprint?PrintBillLine(g_STRREPRINT):1;	

	//����
	GetSalePos()->GetPrintInterface()->nPrintHorLine(); 	

	//��ӡ�ϼ�
    PrintBillTotalSmall();

	//��ӡ��Ӫ������Ʒ����ϼ�
	if (GetSalePos()->GetParams()->m_bPoolClothesMerch) 
		PrintPoolClothesMerch(); 

	m_bReprint?PrintBillLine(g_STRREPRINT):1;

	//ûִ��
	//PrintBillPromoStrInfo();	//��ӡ����������������Ʒ

	//��Ʒӡ�����ٰ���û�д����󣬲���ӡ[dandan.wu 2016-3-25]
	//GetSalePos()->GetPrintInterface()->PrintYinHua(m_vecSaleMerch);

	//����ӡ�����ٰ���û�д����󣬲���ӡ[dandan.wu 2016-3-25]
	//m_strategy_WholeBill_Stamp->print_wholebill_stamp();		   

	//˰�ط�Ʊ 
	if (GetSalePos()->GetParams()->m_nPrintTaxInfo == 1)
	{
		GetSalePos()->GetWriteLog()->WriteLog("��ʼ˰�ط�Ʊ��ӡ",3); 
		CTaxInfoImpl *TaxInfoImpl=GetSalePos()->GetTaxInfoImpl();
		if(m_ServiceImpl.HasSVMerch())
			TaxInfoImpl->PrintInvInfo(PBID_SERVICE_RECEIPT);
		else
			TaxInfoImpl->PrintInvInfo(PBID_TRANS_RECEIPT);
		GetSalePos()->GetWriteLog()->WriteLog("����˰�ط�Ʊ��ӡ",3); 
	} 

	//��ӡƱβ
	PrintBillBootomSmall();

	GetSalePos()->GetWriteLog()->WriteLog("END PrintBillWhole_ONE",4,POSLOG_LEVEL1);//aori move from checkoutdlg::oncheckout 2011-8-30 10:23:53 proj2011-8-30-2 �Ż��ɶ���
	//GetSalePos()->GetPrintInterface()->nSetPrintBusiness(PBID_TRANS_RECEIPT);
}

void CSaleImpl::printBillItemInStock(vector<SaleMerchInfo>& vecSaleMerch)
{
	//��ӡ�����������˳�
	if (!GetSalePos()->GetPrintInterface()->m_bValid) 
	{
		CUniTrace::Trace(_T("��ӡ�ֻ���Ʒ����ӡ��������"));
		return;
	}

	//int nTypeInStock = 1;							//�������Ϊ�ֻ���Ʒ
	int nCount = 0;									//����ֻ���Ʒ������
	char szBuff[256]={0};
	double dbSumInStock = 0.0f;
	double dbSumReal = 0.0f;
	double fSaleQuantity=0.0f,fActualPrice=0.0f;
	vector<SaleMerchInfo>::iterator merchInfo;
	bool bInvoicePrint = false;
	bInvoicePrint = GetSalePos()->m_params.m_bAllowInvoicePrint;
	
	for (merchInfo = vecSaleMerch.begin(); merchInfo != vecSaleMerch.end(); ++merchInfo) 
	{
		//��ȡ����Ʒ����	
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
		CUniTrace::Trace(_T("û���ֻ���Ʒ��Ҫ��ӡ"));
		return;
 	}

	//��ӡ����
	if (!bInvoicePrint)
	{
		GetSalePos()->GetPrintInterface()->nPrintHorLine(' ');
	}
		
	//��ӡ����:�ֻ���Ʒ
	memset(szBuff,0,sizeof(szBuff));
	PrintBillLine(SALEITEM_TITLE_STOCK,DT_LEFT);
	
	//��ӡ����
	if (!bInvoicePrint)
	{
		GetSalePos()->GetPrintInterface()->nPrintHorLine();
	}
	
	//��ӡ��ϸ
	for (merchInfo = vecSaleMerch.begin(); merchInfo != vecSaleMerch.end(); ++merchInfo) 
	{
		if (merchInfo->nMerchState != RS_CANCEL && merchInfo->nItemType == ITEM_IN_STOCK )
		{
			merchInfo->OutPut_Quantity_Price(fSaleQuantity,fActualPrice);

			PrintBillItemSmall(merchInfo,fSaleQuantity,fActualPrice); //aori optimize 2011-10-13 14:53:19 ���д�ӡreplace//PrintBillItemSmall(merchInfo->szPLU,szName,fSaleQuantity,fActualPrice,merchInfo->fSaleAmt); //CString szName;szName.Format("%s%s",merchInfo->nPromoID>0?"(��)":"",merchInfo->szSimpleName);//bug��2011-2-24 10:52:36������ӡ��promoid ������salepromoid�жϡ�
		
			dbSumInStock += fSaleQuantity*fActualPrice;//merchInfo->fSaleAmt_BeforePromotion;
			dbSumReal += merchInfo->fSaleAmt;
		}
	}	

	//��ӡ�ϼ�
	if ( !bInvoicePrint )
	{
		GetSalePos()->GetPrintInterface()->nPrintTotal_m(dbSumInStock,dbSumReal,PBID_TRANS_RECEIPT);
	}
}

void CSaleImpl::printBillItemInOrder(vector<SaleMerchInfo>& vecSaleMerch)
{
	//��ӡ�����������˳�
	if (!GetSalePos()->GetPrintInterface()->m_bValid) 
	{
		CUniTrace::Trace(_T("��ӡ������Ʒ����ӡ��������"));
		return;
	}

	//int nType = 2;								//�������Ϊ������Ʒ
	int nCount = 0;									//����ֻ���Ʒ������
	char m_szBuff[256]={0};
	double dbSum = 0.0f,dbSumReal = 0.0f;			//�ϼ�
	double fSaleQuantity=0.0f,fActualPrice=0.0f;
	vector<CString> vecStrOrderNo;					//��������һ�����֣�û����ĸ
	vector<SaleMerchInfo>::iterator merchInfo;
	vector<CString>::iterator itOrderNo;
	bool bInvoicePrint = false;
	bInvoicePrint = GetSalePos()->m_params.m_bAllowInvoicePrint;
	
	//�������ж����ţ������ظ�
	for ( merchInfo = vecSaleMerch.begin(); merchInfo != vecSaleMerch.end(); ++merchInfo) 
	{
		//��ȡ����Ʒ����	
		if (merchInfo->nMerchState == RS_CANCEL)
		{
			continue;
		}

		//������Ϊ������Ʒ��һ���ж�����
		if ( merchInfo->nItemType == ITEM_IN_ORDER && !merchInfo->strOrderNo.IsEmpty() )
		{
			vecStrOrderNo.push_back(merchInfo->strOrderNo);		
		}
	}	

	int n = 0;

	//������ȥ��
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

		//��ӡ����:���۶�����xxx	
		memset(m_szBuff,0,sizeof(m_szBuff));
		sprintf(m_szBuff, "%s:%s", SALEITEM_TITLE_ORDER,*itOrderNo);
		PrintBillLine(m_szBuff,DT_LEFT);

		//��ӡ����
		if ( !bInvoicePrint )
		{
			GetSalePos()->GetPrintInterface()->nPrintHorLine(); 
		}
		
		//��ӡ��ϸ
		for (merchInfo = vecSaleMerch.begin(); merchInfo != vecSaleMerch.end(); ++merchInfo) 
		{
			if ( merchInfo->nMerchState != RS_CANCEL 
				 && merchInfo->nItemType == ITEM_IN_ORDER 
				 && itOrderNo->Compare(merchInfo->strOrderNo) == 0 )
			{
				merchInfo->OutPut_Quantity_Price(fSaleQuantity,fActualPrice);

				PrintBillItemSmall(merchInfo,fSaleQuantity,fActualPrice); //aori optimize 2011-10-13 14:53:19 ���д�ӡreplace//PrintBillItemSmall(merchInfo->szPLU,szName,fSaleQuantity,fActualPrice,merchInfo->fSaleAmt); //CString szName;szName.Format("%s%s",merchInfo->nPromoID>0?"(��)":"",merchInfo->szSimpleName);//bug��2011-2-24 10:52:36������ӡ��promoid ������salepromoid�жϡ�
	
				//MarkUpֻ��ӡ�ϼ� [dandan.wu 2017-11-1]
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

		//��ӡ�ϼ�
		if ( !bInvoicePrint )
		{
			GetSalePos()->GetPrintInterface()->nPrintTotal_m(dbSum,dbSumReal,PBID_TRANS_RECEIPT);		
		}
	}	
}

void CSaleImpl::printBillBeforeCheckOut()
{
	//��ӡСƱͷ&��Ʒ�У������д�ӡ��
	if (GetSalePos()->m_params.m_nPrinterType==1)
	{
		GetSalePos()->GetWriteLog()->WriteLog("Begin to call CSaleImpl::printBillBeforeCheckOut()",2);

		SortSaleMerch();

		//СƱͷ
		PrintBillHeadSmall();

		m_bReprint?PrintBillLine(g_STRREPRINT):1;//��Ʒ��//int nMerchNum=0;//aori del
	
		//��ӡ�ֻ���Ʒ��ϸ
		printBillItemInStock(m_vecSaleMerchPrePrint);

		//��ӡ������ϸ
		printBillItemInOrder(m_vecSaleMerchPrePrint);

		GetSalePos()->GetWriteLog()->WriteLog("End to call CSaleImpl::printBillBeforeCheckOut()",2);
	}
}

void CSaleImpl::printBillAfterCheckOut()
{
	GetSalePos()->GetWriteLog()->WriteLog("Begin to call CSaleImpl::printBillAfterCheckOut()...",2);

	m_bReprint?PrintBillLine(g_STRREPRINT):1;	
	
	//����
	GetSalePos()->GetPrintInterface()->nPrintHorLine(); 	
	
	//��ӡ�ϼ�
    PrintBillTotalSmall();
	
	//��ӡ��Ӫ������Ʒ����ϼ�
	if (GetSalePos()->GetParams()->m_bPoolClothesMerch) 
		PrintPoolClothesMerch(); 
	
	m_bReprint?PrintBillLine(g_STRREPRINT):1;
	
	//ûִ��
	//PrintBillPromoStrInfo();	//��ӡ����������������Ʒ
	
	//��Ʒӡ��
	//GetSalePos()->GetPrintInterface()->PrintYinHua(m_vecSaleMerch);
	
	//����ӡ��
	//m_strategy_WholeBill_Stamp->print_wholebill_stamp();		   
	
	//˰�ط�Ʊ 
	if (GetSalePos()->GetParams()->m_nPrintTaxInfo == 1)
	{
		GetSalePos()->GetWriteLog()->WriteLog("��ʼ˰�ط�Ʊ��ӡ",3); 
		CTaxInfoImpl *TaxInfoImpl=GetSalePos()->GetTaxInfoImpl();
		if(m_ServiceImpl.HasSVMerch())
			TaxInfoImpl->PrintInvInfo(PBID_SERVICE_RECEIPT);
		else
			TaxInfoImpl->PrintInvInfo(PBID_TRANS_RECEIPT);
		GetSalePos()->GetWriteLog()->WriteLog("����˰�ط�Ʊ��ӡ",3); 
	} 
	
	//��ӡƱβ
	PrintBillBootomSmall();

	GetSalePos()->GetWriteLog()->WriteLog("End to call CSaleImpl::printBillAfterCheckOut()...",2);
}

void CSaleImpl::printBillAboveCancel()
{
	//PrintBillLine("-----------��������-----------");
	GetSalePos()->GetPrintInterface()->nPrintAboveCancel_m(PBID_TRANS_RECEIPT);
}

void CSaleImpl::printBillSumDiscount()
{
	//��ӡ�����������˳�
	if (!GetSalePos()->GetPrintInterface()->m_bValid) 
	{
		CUniTrace::Trace(_T("��ӡ���ۿۣ���ӡ��������"));
		return;
	}
	
	double dbSumDis = 0.0f;				//���Ż�

	double dbSumCondPromDis = 0,dbManualDiscount = 0,dbMemDiscAfterProm = 0;
	double dbSumAmt = 0;

	for ( vector<SaleMerchInfo>::iterator merchInfo = m_vecSaleMerch.begin(); merchInfo != m_vecSaleMerch.end(); ++merchInfo )
	{
		if ( merchInfo->nMerchState != RS_CANCEL ) 
		{
			dbSumAmt += merchInfo->fSaleAmt;
			dbSumCondPromDis += merchInfo->dCondPromDiscount;//��������
			
			//ֻ��ӡMarkDown�ۿۣ���ΪΪ�˹˿����飬MarkUp��Ʒֻ��ӡһ��: PLU MarkUp��ĵ��� �ܼ�[dandan.wu 2017-11-1]
			if ( merchInfo->fManualDiscount > 0 )
			{
				dbManualDiscount += merchInfo->fManualDiscount;
			}

			dbMemDiscAfterProm += merchInfo->dMemDiscAfterProm;//��Ա
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


//��ӡ׷��СƱ(����ϵͳר��)
void CSaleImpl::PrintBillWhole_ADD(vector<SaleMerchInfo>* pVecSaleMerch)
{
	GetSalePos()->GetWriteLog()->WriteLog("BEGIN PrintBillWhole_ADD",4,POSLOG_LEVEL1); 
	GetSalePos()->GetWriteLog()->WriteLog("��ʼ��ӡ����СƱ",2);////��¼��־---------------------	
	//��ӡСƱͷ&��Ʒ�У������д�ӡ��
	//SortSaleMerch();
	PrintBillHeadSmall();//СƱͷ
	m_bReprint?PrintBillLine(g_STRREPRINT):1;//��Ʒ��//int nMerchNum=0;//aori del
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

	GetSalePos()->GetPrintInterface()->nPrintHorLine(); 	//����
    PrintBillTotalSmall(pVecSaleMerch,true);	 //��ӡ�ϼ�
	if (GetSalePos()->GetParams()->m_bPoolClothesMerch,true) //��ӡ��Ӫ������Ʒ����ϼ�
		PrintPoolClothesMerch(pVecSaleMerch); 
	m_bReprint?PrintBillLine(g_STRREPRINT):1;
	GetSalePos()->GetWriteLog()->WriteLog("END PrintBillWhole_ADD",4,POSLOG_LEVEL1);//aori move from checkoutdlg::oncheckout 2011-8-30 10:23:53 proj2011-8-30-2 �Ż��ɶ���
}

//д������־ zenghl 20090608
//����˵��theAll=1 ��ʾ����СƱд����־

int CSaleImpl::WriteSaleLog(bool theAll)
{	
    //�ж��Ƿ���Ҫ��¼������ϸ��¼
	if (GetSalePos()->GetParams()->m_nRecordSaleItem!=1) return -1;//����¼������ϸ��¼
	int Count=m_vecSaleMerch.size(); 
	SaleMerchInfo newMerchInfo("");
	if (theAll)//����СƱ
	{
		if (Count>0)//д�����һ�������������������ϸ
		{
			for (int i=0;i<Count;i++)
			{
				newMerchInfo=m_vecSaleMerch.at(i);
				if (newMerchInfo.nMerchState == RS_CANCEL) continue;//��Ʒȡ������¼
				WriteSaleItemLog(newMerchInfo,0,3);
			}
		}	
	}
	else if (Count>0)//д���һ��������ϸ
	{
		newMerchInfo=m_vecSaleMerch.at(Count-1);
		WriteSaleItemLog(newMerchInfo,0,4);
	}

return 1;
}
//д������ϸ��־ zenghl 20090608
//����˵�� Cancel=1 ��ʾȡ������ȡ������ʾȡ��ĳ�ʽ���
int CSaleImpl::WriteSaleItemLog(SaleMerchInfo &newMerchInfo, bool Cancel,int callflag)
{//aori add 2011-5-13 10:53:09 proj2011-5-11-2�����۲�ƽ5-9���ƴ�3: ��־���� writelogд��dll
	CString strLogBuf, hehe;
	switch(callflag){//aori add 2011-5-11 23:45:00 proj2011-5-11-2�����۲�ƽ5-9���ƴ�
		case 0:hehe.Format("cancel:");
			break;
		case 1:hehe.Format("�߳�%ld ��Ʒ��ϸ Modify_Quantity:",GetCurrentThreadId());
			break;//aori add 2012-3-14 14:20:47  proj 2012-3-5 ��Ϣ����bug 2.��mutex �� OnInputNumber �� OnCheckOut	
		case 2:hehe.Format("��Ʒ��ϸ Modify_Price:");
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
	if (Cancel){//ȡ������������¼��ֵ
		if (newMerchInfo.nManagementStyle==2){//�������Ʒ
			if (newMerchInfo.bFixedPriceAndQuantity)//18��
			{
				strLogBuf.Format("%s %s  %4f %4f %2f",newMerchInfo.szPLU,newMerchInfo.szPLUInput,newMerchInfo.fActualPrice,-newMerchInfo.fSaleQuantity,-newMerchInfo.fSaleAmt);
			}
			else if(newMerchInfo.bPriceInBarcode)//13��
			{
				strLogBuf.Format("%s %s  %4f %d %2f",newMerchInfo.szPLU,newMerchInfo.szPLUInput,newMerchInfo.fActualPrice,-newMerchInfo.nSaleCount,-newMerchInfo.fSaleAmt);
			}else//����
			{
				strLogBuf.Format("%s %s  %4f %d %2f",newMerchInfo.szPLU,newMerchInfo.szPLUInput,newMerchInfo.fActualPrice,-newMerchInfo.nSaleCount,-newMerchInfo.fActualPrice);
			}
		}
		else
		{
			strLogBuf.Format("%s %s  %.4f %d %.2f",newMerchInfo.szPLU,newMerchInfo.szPLUInput,newMerchInfo.fActualPrice,-newMerchInfo.nSaleCount,-newMerchInfo.fActualPrice*newMerchInfo.nSaleCount);
		}
	}else{//��ȡ������
		
		if (newMerchInfo.nManagementStyle==2)//�������Ʒ
		{
			
			if (newMerchInfo.bFixedPriceAndQuantity)//18��
			{
				strLogBuf.Format("%s %s  %4f %4f %2f",newMerchInfo.szPLU,newMerchInfo.szPLUInput,newMerchInfo.fActualPrice,newMerchInfo.fSaleQuantity,newMerchInfo.fSaleAmt);
			}
			else if(newMerchInfo.bPriceInBarcode)//13��
			{
				strLogBuf.Format("%s %s  %4f %d %2f",newMerchInfo.szPLU,newMerchInfo.szPLUInput,newMerchInfo.fActualPrice,newMerchInfo.nSaleCount,newMerchInfo.fSaleAmt);
			}else//����
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
	GetSalePos()->GetWriteLog()->WriteLog(strLogBuf,3,2);//д������־
	return 1;
}

/*
int CSaleImpl::PasswdVaildDay()
{

	CPosClientDB *pDB= CDataBaseManager::getConnection();

	if ( !pDB ){
	GetSalePos()->GetWriteLog()->WriteLog("����posdbʧ��",4);
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
						GetSalePos()->GetWriteLog()->WriteLog("ȡ����������ڳɹ�",4);

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
	GetSalePos()->GetWriteLog()->WriteLog("��ʼ�жϺ����Ƿ��Ѿ�����......",4);
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
	//salesno ���۱�� yyyymmdd0001000010 (yyyymmdd+pos4λ+СƱ5λ+0)
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
	GetSalePos()->GetWriteLog()->WriteLog("�жϺ����Ƿ��Ѿ����ڽ���.",4);
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
	//�޸�24Сʱ���۵����⣬��saledt�޸�Ϊ��Ʊ��ʱ��
//	CString strSaleDT = ::GetFormatDTString(m_stSaleDT);
	CString strSaleDT = ::GetFormatDTString(m_stEndSaleDT);
	const char *szTableDTExt = GetSalePos()->GetTableDTExt(&m_stEndSaleDT);
	double dManualDiscount = 0;

	//��������
	try	{
		/*fTotDiscount=GetSalePos()->GetPromotionInterface()->m_fDisCountAmt;
		//20090526 �޸�Ϊÿ�α�������ʱ����LoadSaleInfo(
		GetSalePos()->GetPromotionInterface()->SetSaleImpl(this);
		GetSalePos()->GetPromotionInterface()->LoadSaleInfo(nSaleID,true);//����������
		*/
	   //ReSortSaleMerch(); zenghl 20090528
		//���浽��������
		//���������ۿ��ܼ�
		for ( merchInfo = m_vecSaleMerch.begin(); merchInfo != m_vecSaleMerch.end(); ++merchInfo ) {
			if ( merchInfo->nMerchState != RS_CANCEL ) {
				//fTotDiscount += merchInfo->fSaleDiscount;//�Ժ�tot���治�ӻ�Ա���� + merchInfo->fMemSaleDiscount;
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
			//30λ�ض�
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
		//�ۿ�ȡ������
		//ReducePrecision(fTotDiscount);
		//��Աinput
		//cardtype=0;
		//MemberCode = "";
		//Get_Member_input();
// 
// 	strSQL.Format("INSERT INTO Sale%s(SaleDT,EndSaleDT,CheckSaleDT,SaleID,HumanID,TotAmt,TotDiscount,Status,PDID,MemberCode,CharCode,ManualDiscount,MemDiscount,UploadFlag,MemIntegral,Shift_No,MemCardNO,MemCardNOType) "
// 				"VALUES ('%s',%d,%d,%ld,%ld,%.4f,%.4f,%d,%d,'%s',%s,%.4f,%.4f,0,%f,%d ,'%s',%d)", 
// 				szTableDTExt, strSaleDT, nEndSaleSpan, nCheckSaleSpan, 
// 				nSaleID, nUserID, fTotAmt, fTotDiscount, 
// 				nSaleStatus, GetSalePos()->GetCurrentTimePeriod(), m_MemberImpl.m_Member.szMemberCode,//GetFieldStringOrNULL(m_MemberImpl.m_Member.szMemberCode), 
// 				strCC.c_str(), m_fDiscount, fTempMemdiscount,m_MemberImpl.m_dMemIntegral,m_uShiftNo ,m_MemberImpl.m_Member.szInputNO,m_MemberImpl.m_Member.nCardNoType);//�ѻ�Ա����������Ҳ�㵽����ȥm_fMemDiscount);
		strSQL.Format("INSERT INTO Sale%s(SaleDT,EndSaleDT,CheckSaleDT,SaleID,HumanID,TotAmt,TotDiscount,Status,PDID,MemberCode,CharCode,ManualDiscount,MemDiscount,UploadFlag,MemIntegral,Shift_No,stampcount,addcol5,MemCardNO,MemCardNOType,AddCol6,AddCol7,AddCol8," 
			" MemberCardType,Ecardno,userID,MemberCardLevel,MemberCardchannel,PromoterID  ) "//���� 2012-9-14 16:07:35 proj 2012-9-10 R2POS T1987 �ܵ�ӡ�� ����
			"VALUES ('%s',%d,%d,%ld,%ld,%.4f,%.4f,%d,%d,'%s',%s,%.4f,%.4f,0,%f,%d,%d,%d ,'%s',%d,'%s','%s','%d' ,'%s','%s','%s','%s','%s','%s')", 
			szTableDTExt, strSaleDT, nEndSaleSpan, nCheckSaleSpan, 
			nSaleID, nUserID, fTotAmt, fTotDiscount, 
			nSaleStatus, GetSalePos()->GetCurrentTimePeriod(),  m_MemberImpl.m_Member.szMemberCode,//GetFieldStringOrNULL(m_MemberImpl.m_Member.szMemberCode), 
			strCC.c_str(), /*m_fDiscount*/dManualDiscount, fTempMemdiscount,m_MemberImpl.m_dMemIntegral,m_uShiftNo,
			/*m_strategy_WholeBill_Stamp->m_YinHuaCount*/0,//�ٰ���û�����ҵ������ֶ�Ӧ����0���������[dandan.wu 2016-3-10]
			m_strategy_WholeBill_Stamp->m_StampGivingMinAmt,
			m_MemberImpl.m_Member.szInputNO,
			m_MemberImpl.m_Member.nCardNoType,strAddCol6/*m_EorderID*/,m_EMemberID,m_EMOrderStatus,
			m_MemberImpl.m_Member.szCardTypeCode,m_MemberImpl.m_Member.szEcardno,m_MemberImpl.m_Member.szUserID,
						m_MemberImpl.m_Member.szMemberCardLevel,m_MemberImpl.m_Member.szMemberCardchannel,m_strPromoterID);//�ѻ�Ա����������Ҳ�㵽����ȥm_fMemDiscount); //��Աƽ̨
		if ( pDB->Execute2(strSQL) == 0)//aori change 2011-3-22 16:51:36 �Ż�clientdb ��ɾ��execute2
			return false;

		//����������ϸ
		int nItemID = 0;
		double fSaleQuantity = 0.0f;
		bool bHasCanceledItems = false;
		double fAddDiscount = GetItemDiscount();

		for ( merchInfo = m_vecSaleMerch.begin(); merchInfo != m_vecSaleMerch.end(); ++merchInfo ) {
			//ȡ����Ʒ�����ڲ�����¼Remark��, FunID = 1 --- ȡ������Ʒ
			if ( merchInfo->nMerchState == RS_CANCEL ) {
				Sleep(5);//�ӳ�1����ִ�У���ֹʱ�������ظ�
				m_pADB->SetOperateRecord(OPT_CANCELITEM, nUserID, merchInfo->szPLU);
				merchInfo->nSID=merchInfo-m_vecSaleMerch.begin();merchInfo->SetCancelSaleItem(m_stEndSaleDT, nSaleID, nUserID,3,NULL);
				bHasCanceledItems = true;
			} else {
				nItemID++;

				double fActualPrice = merchInfo->fActualPrice;
				CString str_fSaleAmt_BeforePromotion;
				str_fSaleAmt_BeforePromotion.Format("%.2f",merchInfo->fSaleAmt_BeforePromotion);
				//����fSaleQuantity
				if ( merchInfo->nManagementStyle != MS_DP ) //�������Ʒ//aori 2011-9-2 17:29:03 ��Ʒ�۸��������� �Ż�
				{
					
					if(merchInfo->bFixedPriceAndQuantity||merchInfo->bPriceInBarcode )//����
					{
						if (	merchInfo->fSaleQuantity >0)
						{ 
							if (merchInfo->nMerchType==5 || merchInfo->nMerchType==6)//��Ӫ�������Ʒ
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
					
					}else//end if ( merchInfo->nManagementStyle != MS_DP ) //�������Ʒ
					{ fSaleQuantity = merchInfo->fSaleQuantity/*nSaleCount*/; }	

			
				} //end if ( merchInfo->nManagementStyle != MS_DP ) //�������Ʒ
				else 
				{
					fSaleQuantity = merchInfo->fSaleQuantity;//(double)merchInfo->nSaleCount;
				}

				//������ֶ����ۣ���ӡ���ֶ��������
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
						//�����ͼ���fSaleQuantity�� [dandan.wu 2016-4-26]
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

				//�����Ʒ���ڴ���������û�е����������Ʒ�Χ�ڣ�����������Ʒ������־
				if(merchInfo->nMerchState == RS_NORMAL && merchInfo->nDiscountStyle != DS_NO)
				{
					merchInfo->nDiscountStyle = DS_NO;
				}
				//�ۿ�ȡ������
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
				
				//����IsDiscountLabel, BackDiscount�ֶ�
				check_lock_saleitem_temp();//aori later 2011-8-7 8:38:11  proj2011-8-3-1 A �� mutex���ж�����checkout֮�� �Ƿ������update�¼�
// 				strSQL.Format("INSERT INTO SaleItem%s(SaleDT,ItemID,SaleID,"
// 					"PLU,RetailPrice,SaleCount,SaleQuantity,SaleAmt,"
// 					"PromoStyle,MakeOrgNO,PromoID, SalesHumanID, NormalPrice,ScanPLU, AddCol4,AddCol2,OrderID,ArtID,OrderItemID,CondPromDiscount,MemDiscAfterProm,ManualDiscount) VALUES ("//,StampCount//aori add 2011-8-30 10:13:21 proj2011-8-30-1 ӡ������
// 					"'%s',%d,%ld,'%s',%.4f,%ld,%.4f,%.4f,%d,%d,%d,%d,%.4f,'%s','%d','%d','%s','%s','%s',%d,%d,%.4f)",//2013-2-27 15:30:11 proj2013-1-23-1 ������Ʒ ���� ��Ա�������޹�,%d//aori add 2011-8-30 10:13:21 proj2011-8-30-1 ӡ������
// 					szTableDTExt, strSaleDT, nItemID, nSaleID, 
// 					merchInfo->szPLU, fActualPrice, merchInfo->nSaleCount, 
// 					fSaleQuantity, merchInfo->fSaleAmt,
// 					merchInfo->nPromoStyle, 
// 					merchInfo->nMakeOrgNO, merchInfo->nPromoID,
// 					merchInfo->nSimplePromoID,merchInfo->fSysPrice,merchInfo->szPLUInput,merchInfo->scantime,merchInfo->IncludeSKU,
// 					merchInfo->strOrderNo,merchInfo->strArtID,merchInfo->strOrderItemID,merchInfo->dCondPromDiscount,merchInfo->dMemDiscAfterProm, merchInfo->fManualDiscount);//,merchInfo->StampCount//aori add 2011-8-30 10:13:21 proj2011-8-30-1 ӡ������
					//����ӡ��Ʊ������Ʒ�ķ�Ʊ��Ϣ���浽�±���[dandan.wu 2016-10-14]
				double dbInvoiceAmt = 0.0;

				if ( GetSalePos()->m_params.m_bAllowInvoicePrint )
				{
					//��Ʊ����Ϊ������Ʒ��Ϊ0 [dandan.wu 2016-11-10]		
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
					") VALUES ('%s',%d,%ld,'%s',%.4f,%ld,%.4f,%.4f,%d,%d,%d,%.4f,%d,%.4f,%.2f,'%s',%d" //%ld to %.4f2013-2-27 15:30:11 proj2013-1-23-1 ������Ʒ ���� ��Ա�������޹�
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
				if ( pDB->Execute2(strSQL) == 0 )//aori change 2011-3-22 16:51:36 �Ż�clientdb ��ɾ��execute2
					return false;
			}
			//����в�����־�����������
			if ( bHasCanceledItems ) {
				GetSalePos()->m_Task.putOperateRecord(m_stEndSaleDT, true);
			}
		}
		//�����ʱ����
		strSQL.Format("DELETE FROM Sale_Temp WHERE HumanID = %d", nUserID);
		pDB->Execute2(strSQL);//aori change 2011-3-22 16:51:36 �Ż�clientdb ��ɾ��execute2

		check_lock_saleitem_temp();strSQL.Format("DELETE FROM SaleItem_Temp WHERE HumanID = %d", nUserID);//aori later 2011-8-7 8:38:11  proj2011-8-3-1 A �� mutex���ж�����checkout֮�� �Ƿ������update�¼�
		pDB->Execute2(strSQL);//aori change 2011-3-22 16:51:36 �Ż�clientdb ��ɾ��execute2
		
	} catch ( CException *pe ) {
		GetSalePos()->GetWriteLog()->WriteLogError(*pe);
		pe->Delete( );
		return false;
	}
	return true;
}

//======================================================================
// �� �� ����GetSaleEndMerchInfo()
// �����������ظ���Ʒ¼��,������¼����Ʒ
// ���������
// ���������void
// �������ڣ�2009-11-29
// �޸����ڣ�
// ��  �ߣ�����
// ����˵����
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
//������¼����Ʒ
// SaleMerchInfo * CSaleImpl::GetSaleEndMerchPoint()2013-2-20 17:33:11 �˳����۱���
// {
// 	return m_pEndSaleMerchInfo;
// }
//������¼����Ʒ�ļ�¼
// void CSaleImpl::EndMerchClear()2013-2-20 17:33:11 �˳����۱���
// {
// 	m_pEndSaleMerchInfo=NULL;
// }


//======================================================================
// �� �� ����PrintBillWHole_cancel 
// ������������ӡ
// ���������
// ���������void
// �������ڣ�2009-12-6 
// �޸����ڣ�
// ��  ��: mbh
// ����˵����
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
		//��ӡ�����������˳�
		if (!GetSalePos()->GetPrintInterface()->m_bValid) return;
		GetSalePos()->GetPrintInterface()->nSetPrintBusiness(PBID_CANCEL_BILL);
		//��¼��־
		GetSalePos()->GetWriteLog()->WriteLog("��ʼ��ӡ����ȡ��СƱ",2);//---------------------	
		//*************thinpon.СƱ��ӡ(ȫ��)**************************
		//thinpon ��ӡСƱС��
		//����������ȴ�ӡСƱͷ����Ʒ��
		CPrintInterface  *printer = GetSalePos()->GetPrintInterface();
		printer->nSetPrintBusiness(PBID_CANCEL_BILL);
		
		if (GetSalePos()->m_params.m_nPrinterType==1)
		{
			//СƱͷ
			//PrintBillHeadSmall();
			printer->nPrintLine(" ");//printer->nPrintLine(" ");//aori 2011-12-30 14:42:38 proj 2011-12-22-1 combine 2.75 with 2.71 saleimpl
			printer->nPrintLine(" ");//printer->nPrintLine(" ");//aori 2011-12-30 14:42:38 proj 2011-12-22-1 combine 2.75 with 2.71 saleimpl

			GetPrintFormatAlignString("***����ȡ��СƱ***", m_szBuff, GetSalePos()->GetParams()->m_nMaxPrintLineLen, DT_CENTER);
			printer->nPrintLine(m_szBuff);//printer->nPrintLine(m_szBuff);//aori 2011-12-30 14:42:38 proj 2011-12-22-1 combine 2.75 with 2.71 saleimpl

			TPrintHead		pPrintHead;
			char *pt = m_szBuff;

			sprintf(m_szBuff, "����:%04d%02d%02d %02d:%02d", 
			m_stSaleDT.wYear, m_stSaleDT.wMonth, m_stSaleDT.wDay, m_stSaleDT.wHour, m_stSaleDT.wMinute);
			pPrintHead.nSaleID=m_nSaleID;			//���һ�ʱ����������ˮ��
			pPrintHead.stSaleDT=m_stSaleDT;	//����/ʱ��	
			pPrintHead.nUserID=atoi(GetSalePos()->GetCasher()->GetUserCode());
			GetSalePos()->GetPrintInterface()->nPrintHead(&pPrintHead);

			for ( vector<SaleMerchInfo>::iterator merchInfo = m_vecSaleMerch.begin(); merchInfo != m_vecSaleMerch.end(); ++merchInfo)
			{
				if (merchInfo->nMerchState != RS_CANCEL)
				{
					double fSaleQuantity=0.0f, fActualPrice=0.0f;
					merchInfo->OutPut_Quantity_Price(fSaleQuantity,fActualPrice);
					PrintBillItemSmall(merchInfo,fSaleQuantity,fActualPrice);//aori optimize 2011-10-13 14:53:19 ���д�ӡ replace //PrintBillItemSmall(merchInfo->szPLU,szName,fSaleQuantity,fActualPrice,merchInfo->fSaleAmt); //szName.Format("%s%s",merchInfo->nPromoID>0?"(��)":"",merchInfo->szSimpleName);//pSaleItem->nSalePromoID//aori limit 2011-2-24 10:52:36
				}
			}
		}
		GetPrintFormatAlignString("***����ȡ��СƱ***", m_szBuff, GetSalePos()->GetParams()->m_nMaxPrintLineLen, DT_CENTER);
		printer->nPrintLine(m_szBuff);//printer->nPrintLine(m_szBuff);//aori 2011-12-30 14:42:38 proj 2011-12-22-1 combine 2.75 with 2.71 saleimpl
		 //��ӡ�ϼ�
		PrintBillTotalSmall();
		//��ӡƱβ
		//PrintBillBootomSmall();
		GetPrintFormatAlignString("***����ȡ��СƱ***", m_szBuff, GetSalePos()->GetParams()->m_nMaxPrintLineLen, DT_CENTER);
		printer->nPrintLine(m_szBuff);//printer->nPrintLine(m_szBuff);//aori 2011-12-30 14:42:38 proj 2011-12-22-1 combine 2.75 with 2.71 saleimpl
		//for (int i=0;i<GetSalePos()->GetParams()->m_nPrintFeedNum;i++)  //mabh
		//{
		//		printer->nPrintLine(" ");//printer->nPrintLine(m_szBuff);//aori 2011-12-30 14:42:38 proj 2011-12-22-1 combine 2.75 with 2.71 saleimpl
		//}
		printer->FeedPaper_End(PBID_TRANS_RECEIPT);

		//��ֽ [dandan.wu 2016-3-8]
		printer->nEndPrint_m();
		GetSalePos()->GetWriteLog()->WriteLog("��ֽ",POSLOG_LEVEL1);
		GetSalePos()->GetWriteLog()->WriteLog("CSaleImpl::PrintBillWHole_cancel(),��ֽ",POSLOG_LEVEL1);

		//delete[] szBuff;
		/*************************************************************/
	//	printer->nSetPrintBusiness(PBID_TRANS_RECEIPT);
	}
	
}

// ��POS���ݿ��ѯ��
// --���۴���
// SELECT plu FROM SaleMerch WHERE MemPromoID IS NOT NULL AND PLU ='��Ʒ��'
// --��������
// SELECT b.ItemCode FROM Promotion a,PromotionItem b 
// WHERE a.MakeOrgNO =b.MakeOrgNO AND a.PromoID =b.PromoID AND a.CustomerRange =2 
// AND GETDATE() BETWEEN a.StartDate AND a.EndDate +1 
// AND ItemCode = '��Ʒ��''
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
{//�޸����ݿ��е�����

	CString strSaleDT = ::GetFormatDTString(m_stEndSaleDT);
	const char *szTableDTExt = GetSalePos()->GetTableDTExt(&m_stEndSaleDT);

	CString strSQL;
	CPosClientDB *pDB = CDataBaseManager::getConnection(true);
	if ( !pDB )
		return false;
	
	try {
		strSQL.Format("UPDATE sale%s set charcode = 'Y' where saleid = %d ",szTableDTExt,m_nSaleID);
		pDB->Execute2( strSQL);//aori change 2011-3-22 16:51:36 �Ż�clientdb ��ɾ��execute2
	
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
	{//�Ƿ��д���������Ϣ
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
// void CSaleImpl::checkXianGouV2(TRIGGER_TYPE_FOR_CHECK triggertype,int nCancelID,bool bQueryBuyingUpLimit){//aori add bQueryBuyingUpLimit 2011-4-26 9:44:43 �޹����⣺ȡ����Ʒʱ����ѯ���Ƿ�������Ʒ
// 	double fSaleAmt=0.0;int bak_Dlginputcount_for_recover=((CPosApp *)AfxGetApp())->GetSaleDlg()->m_nInputCount;//aori add 2011-10-10 15:39:47 proj2011-9-13-1 ��ɨ bug:checkXianGouV2�Ķ���m_nInputCount//���� saleimpl.m_vecSaleMerch ��������item;ˢ��list�����´�ӡ��//bool bneedreprint=FALSE;//aori del 5/10/2011 9:18:37 AM proj2011-5-3-1:���д�ӡ���ƣ������Ա�š� ȡ����Ʒ2
// 	for ( vector<SaleMerchInfo>::iterator merchInfo = m_vecSaleMerch.begin(); merchInfo != m_vecSaleMerch.end(); merchInfo++ ) {//aori change 2011-4-11 11:36:20 bug: ����Ա�ź�ɾ��Ʒ���ͬһ��Ʒ,�޹�����//LimitSaleTrace(i);//aori trace test
// 		if(merchInfo->nMerchState==RS_CANCEL)continue;//aori add 2010-12-29 9:41:25 ȡ����Ʒϵ�д��� //bneedreprint = TRUE;//aori 2011-4-25 15:58:22 aori new change cause bug��ȡ����Ʒʱ �ش�Ʊͷ
// 		SaleMerchInfo tmpmerch=(merchInfo->nSID+1)>m_vecBeforReMem.size()?*merchInfo:m_vecBeforReMem[merchInfo->nSID];//*merchInfo;//aori change 2011-5-11 9:14:43 proj2011-5-3-1:���д�ӡ���ƣ������Ա�š� ȡ����Ʒ 4 �����ȡ����Ʒʱ���۸񲻶�aori add 2011-5-3 11:36:57 proj2011-5-3-1:���д�ӡ���ƣ������Ա�š� ȡ����Ʒ
// 		double nInputCount=merchInfo->bFixedPriceAndQuantity?merchInfo->fSaleQuantity:merchInfo->nSaleCount;//2013-2-27 15:30:11 proj2013-1-23-1 ������Ʒ ���� ��Ա�������޹�
// 		if(0==nInputCount)continue;//aori add 2011-7-12 10:10:47 proj2011-7-5-2 bug: 2.63��
// 		merchInfo->nSaleCount=0;
// 		int nowSID=merchInfo->nSID;((CPosApp *)AfxGetApp())->GetSaleDlg()->m_nCurSID=nowSID;((CPosApp *)AfxGetApp())->GetSaleDlg()->m_nInputCount=nInputCount;//aori add 2011-5-11 9:14:43 proj2011-5-3-1:���д�ӡ���ƣ������Ա�š� ȡ����Ʒ 5 ��bug1��ȡ����Ʒbug//aori re 2011-7-12 10:10:47 proj2011-7-5-2 �˴���->m_nInputCount=nInputCount��ֵ�����ɨ������
// 		m_stratege_xiangou.HandleLimitSaleV3(&merchInfo, (int&)nInputCount,triggertype,nCancelID,bQueryBuyingUpLimit);
// 	
// 		merchInfo->nSaleCount += nInputCount;//aori change move to handleLimitSale
// 		ReCalculate(*merchInfo);  //liaoyu 2012-6-4 ???
// 		//ReCalculate(*merchInfo);//aori del 2012-2-10 10:18:18 proj 2.79 ���Է�������������ʱ Ӧ������δ�ܱ��桢�ָ���Ʒ�Ĵ���ǰsaleAmt��Ϣ��db�����ֶα������Ϣ
// 		fSaleAmt = merchInfo->fSaleAmt;//
// 		IsLimitSale(merchInfo);//aori test
// 		//aori add begin 2011-5-3 11:36:57 proj2011-5-3-1:���д�ӡ���ƣ������Ա�š� ȡ����Ʒ
// 		if(GetSalePos()->m_params.m_nPrinterType==2 
// 		&&triggertype!=TRIGGER_TYPE_InputMemberCode//aori add 2011-5-11 9:14:43 proj2011-5-3-1:���д�ӡ���ƣ������Ա�š� ȡ����Ʒ 3 �������Ա�����޹����ϲ���һ��
// 		&&(merchInfo->nSaleCount!=tmpmerch.nSaleCount|| merchInfo->fActualPrice!=tmpmerch.fActualPrice)){
// 			if (!CanChangePrice()&& CanChangeCount() && !IsDiscountLabelBar)((CPosApp *)AfxGetApp())->GetSaleDlg()->PrintSaleZhuHang(merchInfo);//aori optimize 2011-10-13 14:53:19 ���д�ӡ
// 			PrintCancelItem(tmpmerch);
// 		}//aori add end 2011-5-3 11:36:57 proj2011-5-3-1:���д�ӡ���ƣ������Ա�š� ȡ����Ʒ
// 		SaleMerchInfo merchInfo_log=*merchInfo;
// 		if (merchInfo->nManagementStyle == MS_DP){merchInfo_log.nSaleCount=nInputCount;WriteSaleItemLog(merchInfo_log,0,5);
// 		}else if (merchInfo->nMerchType!=3 && merchInfo->nMerchType!=6 && !( merchInfo->bPriceInBarcode==0  ) ){//aori del && !merchInfo->bFixedPriceAndQuantityproj2013-1-23-1 ������Ʒ ���� ��Ա�������޹�2013-1-23 17:02:40 
// 			if (merchInfo->bPriceInBarcode)merchInfo_log.fSaleQuantity=nInputCount;
// 			WriteSaleItemLog(merchInfo_log,0,5);
// 			}
// 			if ( !CanChangePrice()){
// 				UpdateItemInTemp(merchInfo->nSID);//aori checkxiangou��merchinfo��saleHumanID��ɿ���2011-9-16 10:24:26 proj 2011-9-16-1 2011-9-16 10:26:42 ��Ա+100401��1+282701��60+103224.5 + ȡ��100401ʱ �޹���103224����
// 			}
// 		merchInfo=m_vecSaleMerch.begin()+nowSID;
// 		}
// 	
// 	CSaleDlg *pSaleDlg=((CPosApp *)AfxGetApp())->GetSaleDlg();
// 	pSaleDlg->m_nCurSID=m_vecSaleMerch.size()-1;if(pSaleDlg->m_nCurSID==-1)return;//aori add return 2011-5-13 10:32:55 bug:������ʾ����һ���������������ܼ���Ϣ//aori add2011-4-11 11:36:20 bug: ����Ա�ź�ɾ��Ʒ���ͬһ��Ʒ,�޹����� :
// 	pSaleDlg->m_nInputCount=bak_Dlginputcount_for_recover;//aori add 2011-10-10 15:39:47 proj2011-9-13-1 ��ɨ bug:checkXianGouV2�Ķ���m_nInputCount
// 	pSaleDlg->RefreshWholeList();
// 	pSaleDlg->m_ctrlList.SelectRow(m_vecSaleMerch.size()-1);
// 	double fTotal = GetTotalAmt();
// 	pSaleDlg->m_ctrlShouldIncome.Display( fTotal );	
// 	sprintf(m_szBuff,GetSalePos()->m_params.m_bCDChinese ? "�۸�: %.2f" : "Price: %.2f", fSaleAmt);
// 	sprintf(m_szBuff + 64,GetSalePos()->m_params.m_bCDChinese ? "�ܶ�: %.2f" : "Total: %.2f", fTotal );
// 	pSaleDlg->m_pCD->Display( 2,fTotal );
// 	//if(	bneedreprint)ReprintAll();//aori del 5/10/2011 9:18:37 AM proj2011-5-3-1:���д�ӡ���ƣ������Ա�š� ȡ����Ʒ2
// }
// void CSaleImpl::checkXianGou_bangcheng(TRIGGER_TYPE_FOR_CHECK triggertype,int nCancelID,bool bQueryBuyingUpLimit){//aori add bQueryBuyingUpLimit 2011-4-26 9:44:43 �޹����⣺ȡ����Ʒʱ����ѯ���Ƿ�������Ʒ
// 	double fSaleAmt=0.0;int bak_Dlginputcount_for_recover=((CPosApp *)AfxGetApp())->GetSaleDlg()->m_nInputCount;//aori add 2011-10-10 15:39:47 proj2011-9-13-1 ��ɨ bug:checkXianGouV2�Ķ���m_nInputCount//���� saleimpl.m_vecSaleMerch ��������item;ˢ��list�����´�ӡ��//bool bneedreprint=FALSE;//aori del 5/10/2011 9:18:37 AM proj2011-5-3-1:���д�ӡ���ƣ������Ա�š� ȡ����Ʒ2
// 	for ( vector<SaleMerchInfo>::iterator merchInfo = m_vecSaleMerch.begin(); merchInfo != m_vecSaleMerch.end(); merchInfo++ ) {//aori change 2011-4-11 11:36:20 bug: ����Ա�ź�ɾ��Ʒ���ͬһ��Ʒ,�޹�����//LimitSaleTrace(i);//aori trace test
// 		if(merchInfo->nMerchState==RS_CANCEL)continue;//aori add 2010-12-29 9:41:25 ȡ����Ʒϵ�д��� //bneedreprint = TRUE;//aori 2011-4-25 15:58:22 aori new change cause bug��ȡ����Ʒʱ �ش�Ʊͷ
// 		SaleMerchInfo tmpmerch=(merchInfo->nSID+1)>m_vecBeforReMem.size()?*merchInfo:m_vecBeforReMem[merchInfo->nSID];//*merchInfo;//aori change 2011-5-11 9:14:43 proj2011-5-3-1:���д�ӡ���ƣ������Ա�š� ȡ����Ʒ 4 �����ȡ����Ʒʱ���۸񲻶�aori add 2011-5-3 11:36:57 proj2011-5-3-1:���д�ӡ���ƣ������Ա�š� ȡ����Ʒ
// 		int nowSID=merchInfo->nSID;
// 		((CPosApp *)AfxGetApp())->GetSaleDlg()->m_nCurSID=nowSID;
// 		double nInputCount;
// 		if(	merchInfo->bFixedPriceAndQuantity){//������Ʒ�޹�
// 			nInputCount=merchInfo->fSaleQuantity;//2013-2-27 15:30:11 proj2013-1-23-1 ������Ʒ ���� ��Ա�������޹�
// 			if(0==nInputCount)continue;//aori add 2011-7-12 10:10:47 proj2011-7-5-2 bug: 2.63��
// 			merchInfo->fSaleQuantity=0;
// 			((CPosApp *)AfxGetApp())->GetSaleDlg()->m_nInputCount=1;//aori add 2011-5-11 9:14:43 proj2011-5-3-1:���д�ӡ���ƣ������Ա�š� ȡ����Ʒ 5 ��bug1��ȡ����Ʒbug//aori re 2011-7-12 10:10:47 proj2011-7-5-2 �˴���->m_nInputCount=nInputCount��ֵ�����ɨ������
// 			HandleLimitSale_bangcheng(&merchInfo, nInputCount,triggertype,nCancelID,bQueryBuyingUpLimit);
// 			merchInfo->fSaleQuantity += nInputCount;//aori change move to handleLimitSale
// 
// 		}else{		//��ͨ��Ʒ�޹�
// 			nInputCount=merchInfo->nSaleCount;//2013-2-27 15:30:11 proj2013-1-23-1 ������Ʒ ���� ��Ա�������޹�
// 			if(0==nInputCount)continue;//aori add 2011-7-12 10:10:47 proj2011-7-5-2 bug: 2.63��
// 			merchInfo->nSaleCount=0;
// 			((CPosApp *)AfxGetApp())->GetSaleDlg()->m_nInputCount=nInputCount;//aori add 2011-5-11 9:14:43 proj2011-5-3-1:���д�ӡ���ƣ������Ա�š� ȡ����Ʒ 5 ��bug1��ȡ����Ʒbug//aori re 2011-7-12 10:10:47 proj2011-7-5-2 �˴���->m_nInputCount=nInputCount��ֵ�����ɨ������
// 			HandleLimitSaleV2(&merchInfo, (int&)nInputCount,triggertype,nCancelID,bQueryBuyingUpLimit);
// 			merchInfo->nSaleCount += nInputCount;//aori change move to handleLimitSale
// 		}
// 		ReCalculate(*merchInfo);  //liaoyu 2012-6-4 ???
// 		fSaleAmt = merchInfo->fSaleAmt;//
// 		IsLimitSale(merchInfo);//aori test
// 		//aori add begin 2011-5-3 11:36:57 proj2011-5-3-1:���д�ӡ���ƣ������Ա�š� ȡ����Ʒ
// 		if(GetSalePos()->m_params.m_nPrinterType==2 
// 		&&triggertype!=TRIGGER_TYPE_InputMemberCode//aori add 2011-5-11 9:14:43 proj2011-5-3-1:���д�ӡ���ƣ������Ա�š� ȡ����Ʒ 3 �������Ա�����޹����ϲ���һ��
// 		&&((merchInfo->bFixedPriceAndQuantity?merchInfo->fSaleQuantity!=tmpmerch.fSaleQuantity:merchInfo->nSaleCount!=tmpmerch.nSaleCount)
// 			|| merchInfo->fActualPrice!=tmpmerch.fActualPrice)){
// 			if (!CanChangePrice()&& CanChangeCount() && !IsDiscountLabelBar)((CPosApp *)AfxGetApp())->GetSaleDlg()->PrintSaleZhuHang(merchInfo);//aori optimize 2011-10-13 14:53:19 ���д�ӡ
// 			PrintCancelItem(tmpmerch);
// 		}//aori add end 2011-5-3 11:36:57 proj2011-5-3-1:���д�ӡ���ƣ������Ա�š� ȡ����Ʒ
// 		SaleMerchInfo merchInfo_log=*merchInfo;
// 		if (merchInfo->nManagementStyle == MS_DP){merchInfo_log.nSaleCount=nInputCount;WriteSaleItemLog(merchInfo_log,0,5);
// 		}else if (merchInfo->nMerchType!=3 && merchInfo->nMerchType!=6 && !( merchInfo->bPriceInBarcode==0  ) ){//aori del && !merchInfo->bFixedPriceAndQuantityproj2013-1-23-1 ������Ʒ ���� ��Ա�������޹�2013-1-23 17:02:40 
// 			if (merchInfo->bPriceInBarcode)merchInfo_log.fSaleQuantity=nInputCount;
// 			WriteSaleItemLog(merchInfo_log,0,5);
// 		}
// 		if ( !CanChangePrice())UpdateItemInTemp(merchInfo->nSID);
// 		merchInfo=m_vecSaleMerch.begin()+nowSID;
// 	}	
// 	CSaleDlg *pSaleDlg=((CPosApp *)AfxGetApp())->GetSaleDlg();
// 	pSaleDlg->m_nCurSID=m_vecSaleMerch.size()-1;if(pSaleDlg->m_nCurSID==-1)return;//aori add return 2011-5-13 10:32:55 bug:������ʾ����һ���������������ܼ���Ϣ//aori add2011-4-11 11:36:20 bug: ����Ա�ź�ɾ��Ʒ���ͬһ��Ʒ,�޹����� :
// 	pSaleDlg->m_nInputCount=bak_Dlginputcount_for_recover;//aori add 2011-10-10 15:39:47 proj2011-9-13-1 ��ɨ bug:checkXianGouV2�Ķ���m_nInputCount
// 	pSaleDlg->RefreshWholeList();
// 	pSaleDlg->m_ctrlList.SelectRow(m_vecSaleMerch.size()-1);
// 	double fTotal = GetTotalAmt();
// 	pSaleDlg->m_ctrlShouldIncome.Display( fTotal );	
// 	sprintf(m_szBuff,GetSalePos()->m_params.m_bCDChinese ? "�۸�: %.2f" : "Price: %.2f", fSaleAmt);
// 	sprintf(m_szBuff + 64,GetSalePos()->m_params.m_bCDChinese ? "�ܶ�: %.2f" : "Total: %.2f", fTotal );
// 	pSaleDlg->m_pCD->Display( 2,fTotal );
// 	//if(	bneedreprint)ReprintAll();//aori del 5/10/2011 9:18:37 AM proj2011-5-3-1:���д�ӡ���ƣ������Ա�š� ȡ����Ʒ2
// }
// void CSaleImpl::SetBeMember(bool beMember){//,TMember memberInfo){//aori add
// 	m_bMember = beMember;
// 	//checkXianGouV2(TRIGGER_TYPE_InputMemberCode);//
// 	ReprintAll();//aori add 2011-5-11 9:14:43 proj2011-5-3-1:���д�ӡ���ƣ������Ա�š� ȡ����Ʒ 3 �������Ա�����޹����ϲ���һ��
// }

bool CSaleImpl::IsMember()
{//aori add
	return strlen(GetMember().szMemberCode);
}

// bool CSaleImpl::FindLimitInfo(SaleMerchInfo* salemerchInfo){//aori add
// 	//����������Ʒ��ֱ�ӷ���
// 	//���ڱ�����limit��Ϣ
// 	bool bLocalExist=false;
// 	m_pLimitInfo = NULL;//aori add  2010-12-29 9:41:25 ȡ����Ʒϵ�д���
// 	for(std::vector<LimitSaleInfo>::iterator iterLimInfo = m_VectLimitInfo.begin();iterLimInfo!=m_VectLimitInfo.end();iterLimInfo++){
// 		if(strcmp(iterLimInfo->szPLU,salemerchInfo->szPLU)==0
// 			&& iterLimInfo->bMember==IsMember() // 2010-12-29 8:49:34 ����Ϊ��Ա�޹����ǻ�Ա�޹�
// 			){//
// 			m_pLimitInfo=iterLimInfo;
// 			bLocalExist=true;
// 		}
// 	}
// 	if(!bLocalExist){
// 		//�ڱ����Ҳ���limit��Ϣ������server�˵õ���Ϣ�������浽���ء�
// 		int LimitAlreadySaleCount = 0;
// 		LimitSaleInfo limInfo;
// 		int rtn=	GetSalePos()->m_downloader.GetLimitAlreadySaleCount( salemerchInfo ,m_MemberImpl.m_Member.szMemberCode,LimitAlreadySaleCount);
// 		if (SOCKET_ERROR==rtn){
// 			CString msg;
// 			msg.Format("��ȡ��ʷ�޹���ʱͨѶʧ��,������ʷ��Ϊ0");
// 			//AfxMessageBox(msg);
// 		}
// 		//strcpy(limInfo.szPLU,salemerchInfo->szPLU);
// 		memcpy(limInfo.szPLU,salemerchInfo->szPLU,24);
// 		limInfo.bMember = IsMember();
// 		//limInfo.LimitStyle = salemerchInfo->LimitStyle;//aori test del 2010-12-29 16:55:47 test ȡ�� �޹���Ϣ�е�limitsaleinfo����limitstyle
// 		//limInfo.LimitQty = salemerchInfo->LimitQty;
// 		limInfo.LimitLEFTcount=(LimitAlreadySaleCount < salemerchInfo->LimitQty?salemerchInfo->LimitQty - LimitAlreadySaleCount:0);
// 		m_VectLimitInfo.push_back(limInfo);
// 		m_pLimitInfo=&m_VectLimitInfo[m_VectLimitInfo.size()-1];
// 	}	
// 	return bLocalExist;
// }
// void CSaleImpl::HandleLimitSaleV2(vector<SaleMerchInfo>::iterator * ppsalemerchInfo,int& nInputCount,//2013-2-27 15:30:11 proj2013-1-23-1 ������Ʒ ���� ��Ա�������޹�
// 								 TRIGGER_TYPE_FOR_CHECK TriggerForCheck,int nCancelID,bool bQueryBuyingUpLimit){//aori add bcheck �Ƿ���ȫɨ�裨member�仯||ȡ���޹���Ʒʱ����ȫɨ����bcancellimitmerch�Ƿ���ȡ���޹���Ʒ//aori add bQueryBuyingUpLimit 2011-4-26 9:44:43 �޹����⣺ȡ����Ʒʱ����ѯ���Ƿ�������Ʒ
// 	vector<SaleMerchInfo>::iterator salemerchInfo=*ppsalemerchInfo;
// 	if(!((IsMember() 
// 			&& (salemerchInfo->LimitStyle>LIMITSTYLE_NoLimit)//&&salemerchInfo->LimitStyle<=LIMITSTYLE_UPLIMIT)//aori add 2010-12-31 17:13:33 �������޹����ͣ�5����Ա���� 6���ǻ�Ա���� 7����Աȫ�峬��
// 					&& TRIGGER_TYPE_NoCheck==TriggerForCheck)
// 		||(!IsMember() 
// 			&&(LIMITSTYLE_ALLXIAOPIAO<=salemerchInfo->LimitStyle) //aori add 2010-12-31 17:13:33 �������޹����ͣ�5����Ա���� 6���ǻ�Ա���� 7����Աȫ�峬��
// 					&& TRIGGER_TYPE_NoCheck==TriggerForCheck)
// 		||(salemerchInfo->LimitStyle>LIMITSTYLE_NoLimit&&salemerchInfo->LimitStyle<=LIMITSTYLE_ALLXIAOPIAO
// 				&& CHECKEDHISTORY_NoCheck==salemerchInfo->CheckedHistory 
// 					&& TRIGGER_TYPE_InputMemberCode==TriggerForCheck)
// 		||( (LIMITSTYLE_ALLXIAOPIAO<=salemerchInfo->LimitStyle)//aori add ���ӶԳ�����Ʒ��handle ��Ϊ��2010-12-31 17:13:33 need ��limitstyle==4��ȫ��СƱ�޹���&& bmember && memprice==retailprice ʱ ������Ʒ actualpriceӦȡNormalprice
// 				&& CHECKEDHISTORY_NoMemLimit==salemerchInfo->CheckedHistory 
// 					&& TRIGGER_TYPE_InputMemberCode==TriggerForCheck)
// 		||( LIMITSTYLE_UPLIMIT<=salemerchInfo->LimitStyle//aori add 2010-12-31 17:13:33 �������޹����ͣ�5����Ա���� 6���ǻ�Ա���� 7����Աȫ�峬��
// 					&& TRIGGER_TYPE_CancelLimitSale==TriggerForCheck)
// 	)||salemerchInfo->bIsDiscountLabel ){//aori delproj2013-1-23-1 ������Ʒ ���� ��Ա�������޹�2013-1-23 17:02:40  ||salemerchInfo->bFixedPriceAndQuantity//aori add 2010-12-30 14:30:39 ���ʲ�Ӧ�òμ��޹�
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
// 	//if(salemerchInfo->nSaleCount+nSaleCount<=m_pLimitInfo->LimitLEFTcount){//�����������ڿ��޹���Χ��,������������merchinfoֱ�ӷ���,���±���limitinfo
// 	if(nInputCount<=m_pLimitInfo->LimitLEFTcount){//�����������ڿ��޹���Χ��,������������merchinfoֱ�ӷ���,���±���limitinfo
// 		m_pLimitInfo->LimitLEFTcount -=nInputCount;
// 		return;
// 
// 	}else{//�����µ�����merchinfo���������޹�merchinfo��Ȼ����salemerchinfoָ���µ�merchinfo������nSaleCountֵ
// 		//if(TRIGGER_TYPE_CancelLimitSale!=TriggerForCheck){//aori del ȡ����Ʒ���������� ��ʱ��� ��ȫɨ�����
// 		if(bQueryBuyingUpLimit){//aori add bQueryBuyingUpLimit 2011-4-26 9:44:43 �޹����⣺ȡ����Ʒʱ����ѯ���Ƿ�������Ʒ
// 			CConfirmLimitSaleDlg confirmDlg;
// 			confirmDlg.m_strConfirm.Format("�ѳ��ޣ��Ƿ����޲�����Ʒ");
// 			if ( confirmDlg.DoModal() == IDCANCEL ) {
// 				nInputCount=m_pLimitInfo->LimitLEFTcount;
// 				m_pLimitInfo->LimitLEFTcount -=nInputCount;
// 				return;
// 			}//aori add end 2010-12-25 13:51:48 later ѯ���Ƿ�������Ʒ��
// 		}
// 		double oldandnewTOTALCOUNT=nInputCount;
// 		CSaleDlg *pSaleDlg=((CPosApp *)AfxGetApp())->GetSaleDlg();
// 		(salemerchInfo->nSaleCount)+=(m_pLimitInfo->LimitLEFTcount);//bug???aori later LimitLEFTcountΪ��ʱ???
// 		if(salemerchInfo->nSaleCount){//
// 			ReCalculate(*salemerchInfo);
// 			IsLimitSale(salemerchInfo);//aori test
// 			SaleMerchInfo merchInfo_log=*salemerchInfo;
// 			if (salemerchInfo->nManagementStyle == MS_DP){
// 				merchInfo_log.nSaleCount=m_pLimitInfo->LimitLEFTcount;WriteSaleItemLog(merchInfo_log,0,6);
// 				}else if (salemerchInfo->bPriceInBarcode&&salemerchInfo->nMerchType!=3 && salemerchInfo->nMerchType!=6 && !( salemerchInfo->bPriceInBarcode==0  ) ){//proj2013-1-23-1 ������Ʒ ���� ��Ա�������޹�2013-1-23 17:02:40 aori del && !salemerchInfo->bFixedPriceAndQuantity
// 				merchInfo_log.fSaleQuantity=m_pLimitInfo->LimitLEFTcount;WriteSaleItemLog(merchInfo_log,0,6);
// 			}		
// 			nInputCount = salemerchInfo->nSaleCount;//�޹�����Ʒ��������Ҫ����ӡ������ԭ�����������޹��ͳ����޹����ܺ͡����nSaleCount����m_nsaleCount�����á�
// 			pSaleDlg->RefreshList(salemerchInfo->nSID);//pSaleDlg->m_nCurSID);	
// 			pSaleDlg->m_nInputCount=m_pLimitInfo->LimitLEFTcount;//aori add 2011-4-26 15:56:03 ���д�ӡ�¹���Ʒbug
// 				if ( !CanChangePrice()){
// 				UpdateItemInTemp(salemerchInfo->nSID);//pSaleDlg->m_nCurSID);
// 				if (GetSalePos()->m_params.m_nPrinterType==2
// 				&&TriggerForCheck!=TRIGGER_TYPE_InputMemberCode)//aori add 2011-5-11 9:14:43 proj2011-5-3-1:���д�ӡ���ƣ������Ա�š� ȡ����Ʒ 3 �������Ա�����޹����ϲ���һ��
// 					pSaleDlg->PrintSaleZhuHang(salemerchInfo);//aori optimize 2011-10-13 14:53:19 ���д�ӡ//aori change 5/10/2011 9:18:37 AM proj2011-5-3-1:���д�ӡ���ƣ������Ա�š� ȡ����Ʒ2
// 			}
// 			pSaleDlg->m_nInputCount=oldandnewTOTALCOUNT-m_pLimitInfo->LimitLEFTcount;//aori del 2011-4-26 15:37:07 del test m_nSameMerchContinueInputCount
// 		}
// 		//��������merchinfo�������޹������merchinfo
// 		pSaleDlg->m_nCurSID = AddUPLimitsalemerch(&salemerchInfo,salemerchInfo->nSaleCount==0?FALSE:TRUE);//��������:�����µ�������Ʒmerchinfo�����غ�salemerchInfoָ���µ�������Ʒmerchinfo������ֵ����m_nCurSID��Ȼ�����nsalecount
// 		nInputCount=oldandnewTOTALCOUNT-m_pLimitInfo->LimitLEFTcount;//����saledlg��nSaleCount ��nSaleCount����������Ʒ�ģ�������modify������Ʒ��
// 		*ppsalemerchInfo=salemerchInfo;
// 		m_pLimitInfo->LimitLEFTcount=0;
// 	}
// }
// void CSaleImpl::HandleLimitSaleV3(vector<SaleMerchInfo>::iterator * ppsalemerchInfo,int& nInputCount,//2013-2-27 15:30:11 proj2013-1-23-1 ������Ʒ ���� ��Ա�������޹�
// 								 TRIGGER_TYPE_FOR_CHECK TriggerForCheck,int nCancelID,bool bQueryBuyingUpLimit){//aori add bcheck �Ƿ���ȫɨ�裨member�仯||ȡ���޹���Ʒʱ����ȫɨ����bcancellimitmerch�Ƿ���ȡ���޹���Ʒ//aori add bQueryBuyingUpLimit 2011-4-26 9:44:43 �޹����⣺ȡ����Ʒʱ����ѯ���Ƿ�������Ʒ
// 	vector<SaleMerchInfo>::iterator salemerchInfo=*ppsalemerchInfo;
// 	if(!((IsMember() 
// 			&& (salemerchInfo->LimitStyle>LIMITSTYLE_NoLimit)//&&salemerchInfo->LimitStyle<=LIMITSTYLE_UPLIMIT)//aori add 2010-12-31 17:13:33 �������޹����ͣ�5����Ա���� 6���ǻ�Ա���� 7����Աȫ�峬��
// 					&& TRIGGER_TYPE_NoCheck==TriggerForCheck)
// 		||(!IsMember() 
// 			&&(LIMITSTYLE_ALLXIAOPIAO<=salemerchInfo->LimitStyle) //aori add 2010-12-31 17:13:33 �������޹����ͣ�5����Ա���� 6���ǻ�Ա���� 7����Աȫ�峬��
// 					&& TRIGGER_TYPE_NoCheck==TriggerForCheck)
// 		||(salemerchInfo->LimitStyle>LIMITSTYLE_NoLimit&&salemerchInfo->LimitStyle<=LIMITSTYLE_ALLXIAOPIAO
// 				&& CHECKEDHISTORY_NoCheck==salemerchInfo->CheckedHistory 
// 					&& TRIGGER_TYPE_InputMemberCode==TriggerForCheck)
// 		||( (LIMITSTYLE_ALLXIAOPIAO<=salemerchInfo->LimitStyle)//aori add ���ӶԳ�����Ʒ��handle ��Ϊ��2010-12-31 17:13:33 need ��limitstyle==4��ȫ��СƱ�޹���&& bmember && memprice==retailprice ʱ ������Ʒ actualpriceӦȡNormalprice
// 				&& CHECKEDHISTORY_NoMemLimit==salemerchInfo->CheckedHistory 
// 					&& TRIGGER_TYPE_InputMemberCode==TriggerForCheck)
// 		||( LIMITSTYLE_UPLIMIT<=salemerchInfo->LimitStyle//aori add 2010-12-31 17:13:33 �������޹����ͣ�5����Ա���� 6���ǻ�Ա���� 7����Աȫ�峬��
// 					&& TRIGGER_TYPE_CancelLimitSale==TriggerForCheck)
// 	)||salemerchInfo->bIsDiscountLabel ){//aori delproj2013-1-23-1 ������Ʒ ���� ��Ա�������޹�2013-1-23 17:02:40  ||salemerchInfo->bFixedPriceAndQuantity//aori add 2010-12-30 14:30:39 ���ʲ�Ӧ�òμ��޹�
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
// 	//if(salemerchInfo->nSaleCount+nSaleCount<=m_pLimitInfo->LimitLEFTcount){//�����������ڿ��޹���Χ��,������������merchinfoֱ�ӷ���,���±���limitinfo
// 	if(nInputCount<=m_pLimitInfo->LimitLEFTcount){//�����������ڿ��޹���Χ��,������������merchinfoֱ�ӷ���,���±���limitinfo
// 		m_pLimitInfo->LimitLEFTcount -=nInputCount;
// 		return;
// 
// 	}else{
// 		if(bQueryBuyingUpLimit){//aori add bQueryBuyingUpLimit 2011-4-26 9:44:43 �޹����⣺ȡ����Ʒʱ����ѯ���Ƿ�������Ʒ
// 			CConfirmLimitSaleDlg confirmDlg;confirmDlg.m_strConfirm.Format("�ѳ��ޣ��Ƿ����޲�����Ʒ");
// 			if ( confirmDlg.DoModal() == IDCANCEL ) {
// 				nInputCount=m_pLimitInfo->LimitLEFTcount;
// 				m_pLimitInfo->LimitLEFTcount =0;
// 				return;
// 			}//aori add end 2010-12-25 13:51:48 later ѯ���Ƿ�������Ʒ��
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
// 		(salemerchInfo->nSaleCount)+=(m_pLimitInfo->LimitLEFTcount);//bug???aori later LimitLEFTcountΪ��ʱ???
// 		if(salemerchInfo->nSaleCount){//
// 			ReCalculate(*salemerchInfo);
// 			IsLimitSale(salemerchInfo);//aori test
// 			SaleMerchInfo merchInfo_log=*salemerchInfo;
// 			if (salemerchInfo->nManagementStyle == MS_DP){
// 				merchInfo_log.nSaleCount=m_pLimitInfo->LimitLEFTcount;WriteSaleItemLog(merchInfo_log,0,6);
// 				}else if (salemerchInfo->bPriceInBarcode&&salemerchInfo->nMerchType!=3 && salemerchInfo->nMerchType!=6 && !( salemerchInfo->bPriceInBarcode==0  ) ){//proj2013-1-23-1 ������Ʒ ���� ��Ա�������޹�2013-1-23 17:02:40 aori del && !salemerchInfo->bFixedPriceAndQuantity
// 				merchInfo_log.fSaleQuantity=m_pLimitInfo->LimitLEFTcount;WriteSaleItemLog(merchInfo_log,0,6);
// 			}		
// 			nInputCount = salemerchInfo->nSaleCount;//�޹�����Ʒ��������Ҫ����ӡ������ԭ�����������޹��ͳ����޹����ܺ͡����nSaleCount����m_nsaleCount�����á�
// 			pSaleDlg->RefreshList(salemerchInfo->nSID);//pSaleDlg->m_nCurSID);	
// 			pSaleDlg->m_nInputCount=m_pLimitInfo->LimitLEFTcount;//aori add 2011-4-26 15:56:03 ���д�ӡ�¹���Ʒbug
// 				if ( !CanChangePrice()){
// 				UpdateItemInTemp(salemerchInfo->nSID);//pSaleDlg->m_nCurSID);
// 				if (GetSalePos()->m_params.m_nPrinterType==2
// 				&&TriggerForCheck!=TRIGGER_TYPE_InputMemberCode)//aori add 2011-5-11 9:14:43 proj2011-5-3-1:���д�ӡ���ƣ������Ա�š� ȡ����Ʒ 3 �������Ա�����޹����ϲ���һ��
// 					pSaleDlg->PrintSaleZhuHang(salemerchInfo);//aori optimize 2011-10-13 14:53:19 ���д�ӡ//aori change 5/10/2011 9:18:37 AM proj2011-5-3-1:���д�ӡ���ƣ������Ա�š� ȡ����Ʒ2
// 			}
// 			pSaleDlg->m_nInputCount=oldandnewTOTALCOUNT-m_pLimitInfo->LimitLEFTcount;//aori del 2011-4-26 15:37:07 del test m_nSameMerchContinueInputCount
// 		}
// 		//��������merchinfo�������޹������merchinfo
// 		pSaleDlg->m_nCurSID = AddUPLimitsalemerch(&salemerchInfo,salemerchInfo->nSaleCount==0?FALSE:TRUE);//��������:�����µ�������Ʒmerchinfo�����غ�salemerchInfoָ���µ�������Ʒmerchinfo������ֵ����m_nCurSID��Ȼ�����nsalecount
// 		nInputCount=oldandnewTOTALCOUNT-m_pLimitInfo->LimitLEFTcount;//����saledlg��nSaleCount ��nSaleCount����������Ʒ�ģ�������modify������Ʒ��
// 		*ppsalemerchInfo=salemerchInfo;
// 		m_pLimitInfo->LimitLEFTcount=0;
// 	}
// }
// 
// void CSaleImpl::HandleLimitSale_bangcheng(vector<SaleMerchInfo>::iterator * ppsalemerchInfo,double& YYquantity,//2013-2-27 15:30:11 proj2013-1-23-1 ������Ʒ ���� ��Ա�������޹�
// 								 TRIGGER_TYPE_FOR_CHECK TriggerForCheck,int nCancelID,bool bQueryBuyingUpLimit){//aori add bcheck �Ƿ���ȫɨ�裨member�仯||ȡ���޹���Ʒʱ����ȫɨ����bcancellimitmerch�Ƿ���ȡ���޹���Ʒ//aori add bQueryBuyingUpLimit 2011-4-26 9:44:43 �޹����⣺ȡ����Ʒʱ����ѯ���Ƿ�������Ʒ
// 	vector<SaleMerchInfo>::iterator salemerchInfo=*ppsalemerchInfo;
// 	if(!((IsMember() 
// 			&& (salemerchInfo->LimitStyle>LIMITSTYLE_NoLimit)//&&salemerchInfo->LimitStyle<=LIMITSTYLE_UPLIMIT)//aori add 2010-12-31 17:13:33 �������޹����ͣ�5����Ա���� 6���ǻ�Ա���� 7����Աȫ�峬��
// 					&& TRIGGER_TYPE_NoCheck==TriggerForCheck)
// 		||(!IsMember() 
// 			&&(LIMITSTYLE_ALLXIAOPIAO<=salemerchInfo->LimitStyle) //aori add 2010-12-31 17:13:33 �������޹����ͣ�5����Ա���� 6���ǻ�Ա���� 7����Աȫ�峬��
// 					&& TRIGGER_TYPE_NoCheck==TriggerForCheck)
// 		||(salemerchInfo->LimitStyle>LIMITSTYLE_NoLimit&&salemerchInfo->LimitStyle<=LIMITSTYLE_ALLXIAOPIAO
// 				&& CHECKEDHISTORY_NoCheck==salemerchInfo->CheckedHistory 
// 					&& TRIGGER_TYPE_InputMemberCode==TriggerForCheck)
// 		||( (LIMITSTYLE_ALLXIAOPIAO<=salemerchInfo->LimitStyle)//aori add ���ӶԳ�����Ʒ��handle ��Ϊ��2010-12-31 17:13:33 need ��limitstyle==4��ȫ��СƱ�޹���&& bmember && memprice==retailprice ʱ ������Ʒ actualpriceӦȡNormalprice
// 				&& CHECKEDHISTORY_NoMemLimit==salemerchInfo->CheckedHistory 
// 					&& TRIGGER_TYPE_InputMemberCode==TriggerForCheck)
// 		||( LIMITSTYLE_UPLIMIT<=salemerchInfo->LimitStyle//aori add 2010-12-31 17:13:33 �������޹����ͣ�5����Ա���� 6���ǻ�Ա���� 7����Աȫ�峬��
// 					&& TRIGGER_TYPE_CancelLimitSale==TriggerForCheck)
// 	)||salemerchInfo->bIsDiscountLabel ){//aori delproj2013-1-23-1 ������Ʒ ���� ��Ա�������޹�2013-1-23 17:02:40  ||salemerchInfo->bFixedPriceAndQuantity//aori add 2010-12-30 14:30:39 ���ʲ�Ӧ�òμ��޹�
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
// 	if(YYquantity<=m_pLimitInfo->LimitLEFTcount){//�����������ڿ��޹���Χ��,������������merchinfoֱ�ӷ���,���±���limitinfo
// 		m_pLimitInfo->LimitLEFTcount -=YYquantity;
// 		return;
// 
// 	}else{//�����µ�����merchinfo���������޹�merchinfo��Ȼ����salemerchinfoָ���µ�merchinfo������nSaleCountֵ
// 		//if(TRIGGER_TYPE_CancelLimitSale!=TriggerForCheck){//aori del ȡ����Ʒ���������� ��ʱ��� ��ȫɨ�����
// 		if(bQueryBuyingUpLimit){//aori add bQueryBuyingUpLimit 2011-4-26 9:44:43 �޹����⣺ȡ����Ʒʱ����ѯ���Ƿ�������Ʒ
// 			CConfirmLimitSaleDlg confirmDlg;confirmDlg.m_strConfirm.Format("�ѳ��ޣ���:���޲��ְ������ۼƼۣ���:��������Ʒ");if ( confirmDlg.DoModal() == IDCANCEL ) {
// 				YYquantity=m_pLimitInfo->LimitLEFTcount;
// 				m_pLimitInfo->LimitLEFTcount -=YYquantity;
// 				return;
// 			}//aori add end 2010-12-25 13:51:48 later ѯ���Ƿ�������Ʒ��
// 		}
// 		double oldandnewTOTALquantity=YYquantity;
// 		CSaleDlg *pSaleDlg=((CPosApp *)AfxGetApp())->GetSaleDlg();
// 		(salemerchInfo->fSaleQuantity)=(m_pLimitInfo->LimitLEFTcount);//bug???aori later LimitLEFTcountΪ��ʱ???
// 		if(salemerchInfo->fSaleQuantity){//
// 			ReCalculate(*salemerchInfo);
// 			IsLimitSale(salemerchInfo);//aori test
// 			SaleMerchInfo merchInfo_log=*salemerchInfo;
// 			if (salemerchInfo->nManagementStyle == MS_DP){
// 				merchInfo_log.fSaleQuantity=m_pLimitInfo->LimitLEFTcount;WriteSaleItemLog(merchInfo_log,0,6);
// 				}else if (salemerchInfo->bPriceInBarcode&&salemerchInfo->nMerchType!=3 && salemerchInfo->nMerchType!=6 && !( salemerchInfo->bPriceInBarcode==0  ) ){//proj2013-1-23-1 ������Ʒ ���� ��Ա�������޹�2013-1-23 17:02:40 aori del && !salemerchInfo->bFixedPriceAndQuantity
// 				merchInfo_log.fSaleQuantity=m_pLimitInfo->LimitLEFTcount;WriteSaleItemLog(merchInfo_log,0,6);
// 			}		
// 			YYquantity = salemerchInfo->fSaleQuantity;//�޹�����Ʒ��������Ҫ����ӡ������ԭ�����������޹��ͳ����޹����ܺ͡����nSaleCount����m_nsaleCount�����á�
// 			pSaleDlg->RefreshList(salemerchInfo->nSID);//pSaleDlg->m_nCurSID);	
// 			//pSaleDlg->m_nInputCount=m_pLimitInfo->LimitLEFTcount;//aori add 2011-4-26 15:56:03 ���д�ӡ�¹���Ʒbug
// 			if ( !CanChangePrice()){
// 				UpdateItemInTemp(salemerchInfo->nSID);//pSaleDlg->m_nCurSID);
// 				if (GetSalePos()->m_params.m_nPrinterType==2
// 				&&TriggerForCheck!=TRIGGER_TYPE_InputMemberCode)//aori add 2011-5-11 9:14:43 proj2011-5-3-1:���д�ӡ���ƣ������Ա�š� ȡ����Ʒ 3 �������Ա�����޹����ϲ���һ��
// 					pSaleDlg->PrintSaleZhuHang(salemerchInfo);//aori optimize 2011-10-13 14:53:19 ���д�ӡ//aori change 5/10/2011 9:18:37 AM proj2011-5-3-1:���д�ӡ���ƣ������Ա�š� ȡ����Ʒ2
// 			}
// 			pSaleDlg->m_nInputCount=oldandnewTOTALquantity-m_pLimitInfo->LimitLEFTcount;//aori del 2011-4-26 15:37:07 del test m_nSameMerchContinueInputCount
// 		}
// 		//��������merchinfo�������޹������merchinfo
// 		pSaleDlg->m_nCurSID = AddUPLimitsalemerch(&salemerchInfo,salemerchInfo->fSaleQuantity==0?FALSE:TRUE);//��������:�����µ�������Ʒmerchinfo�����غ�salemerchInfoָ���µ�������Ʒmerchinfo������ֵ����m_nCurSID��Ȼ�����nsalecount
// 		YYquantity=oldandnewTOTALquantity-m_pLimitInfo->LimitLEFTcount;//����saledlg��nSaleCount ��nSaleCount����������Ʒ�ģ�������modify������Ʒ��
// 		*ppsalemerchInfo=salemerchInfo;
// 		m_pLimitInfo->LimitLEFTcount=0;
// 	}
// }
void CSaleImpl::IsLimitSale(SaleMerchInfo* merchInfo){//aori add
	if(IsMember()&&merchInfo->LimitStyle>LIMITSTYLE_NoLimit&&merchInfo->LimitStyle<LIMITSTYLE_UPLIMIT
 		|| !IsMember()&&LIMITSTYLE_ALLXIAOPIAO==merchInfo->LimitStyle
 		|| LIMITSTYLE_UPLIMIT <= merchInfo->LimitStyle////aori add 2010-12-31 17:13:33 �������޹����ͣ�5����Ա���� 6���ǻ�Ա���� 7����Աȫ�峬��
	){
		merchInfo->bLimitSale=true;
	}
	else 
		merchInfo->bLimitSale=false;
	merchInfo->bMember=IsMember();
}

//void CSaleImpl::PrintBillItemSmall(CString item_no, CString item_name, double qty, double price, double amnt)//aori add ֧���޹���������Ʒ�����ִ�ӡ
void CSaleImpl::PrintBillItemSmall(const SaleMerchInfo *pmerchinfo, double qty)
{//aori add ֧���޹���������Ʒ�����ִ�ӡ
	if (!GetSalePos()->GetPrintInterface()->m_bValid) 
		return;
	SaleMerchInfo saleInfo=*pmerchinfo;
	saleInfo.nSaleCount	=qty;
	saleInfo.fSaleQuantity=qty;
	saleInfo.fSaleAmt	=saleInfo.fSaleAmt_BeforePromotion=qty*saleInfo.fActualPrice;//aori add 2012-2-7 16:11:04 proj 2.79 ���Է�������������ʱ ������ �� fSaleAmt_BeforePromotion

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
// 		if( merchInfo->nSaleCount <= m_pLimitInfo->LimitLEFTcount){//�����������ڿ��޹���Χ��,������������merchinfoֱ�ӷ���,���±���limitinfo
// 			m_pLimitInfo->LimitLEFTcount -= merchInfo->nSaleCount;
// 		}else
// 			m_pLimitInfo->LimitLEFTcount=0;
// 	}
// }
void CSaleImpl::HideBankCardNum(const PayInfo* pay,TPaymentItem& pPaymentItem)
{
	//aori add ���п�������icbc��������ֵ�� ��ӡ���ε�����5������8λ
	//Modified by liaoyu 2013-7-12 : ���ų���>=16, ����ǰ6λ����4λ�������м��ַ�
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

void CSaleImpl::PrintCancelItem(SaleMerchInfo saleInfo){//aori add 2011-5-3 11:36:57 proj2011-5-3-1:���д�ӡ���ƣ������Ա�š� ȡ����Ʒ
	saleInfo.fSaleAmt=saleInfo.fSaleAmt_BeforePromotion=-saleInfo.fSaleQuantity/*nSaleCount*/*saleInfo.fActualPrice;//aori add 2012-2-7 16:11:04 proj 2.79 ���Է�������������ʱ ������ �� fSaleAmt_BeforePromotion
	saleInfo.fSaleQuantity=-saleInfo.fSaleQuantity/*nSaleCount*/;
	if(m_ServiceImpl.HasSVMerch())
		GetSalePos()->GetPrintInterface()->nPrintSalesItem_m(&saleInfo,PBID_SERVICE_RECEIPT);
	else
		GetSalePos()->GetPrintInterface()->nPrintSalesItem_m(&saleInfo,PBID_TRANS_RECEIPT);
}
void CSaleImpl::ReprintAll(){//aori add 5/10/2011 9:18:37 AM proj2011-5-3-1:���д�ӡ���ƣ������Ա�š� ȡ����Ʒ2
	if (GetSalePos()->m_params.m_nPrinterType==2){
		GetSalePos()->GetPrintInterface()->nPrintLine_m( "      ***����СƱ����***\n\n\n\n",PBID_TRANS_RECEIPT);//GetSalePos()->GetPrintInterface()->nPrintLine( "      ***����СƱ����***\n\n\n\n" );2011-12-30 14:42:38 proj 2011-12-22-1 combine 2.75 with 2.71 saleimpl
		PrintBillHeadSmall();//��ӡСƱ̨ͷ 	
		for ( vector<SaleMerchInfo>::iterator merchInfo = m_vecSaleMerch.begin(); merchInfo != m_vecSaleMerch.end(); ++merchInfo )
		{
			if ( merchInfo->nMerchState != RS_CANCEL)
			   ((CPosApp *)AfxGetApp())->GetSaleDlg()->PrintSaleZhuHang(merchInfo,true);
		}
	}	

}
bool CSaleImpl::m_bDuringLock_saleitemtemp=false;
void CSaleImpl::lock_saleitem_temp()
{	return;//aori add proj 2011-8-3-1 ���۲�ƽ 2011-10-31 15:42:43 �޸��߼� �ƹ���ƽ���⣬ȡ����ƽ��־
	HANDLE hhandle=CreateMutex(NULL,0,"bDuringLock_saleitemtemp");//aori add 2012-2-17 16:27:19 proj 2012-2-13 �ۿ۱�ǩУ�� ȥ����־�������
	WaitForSingleObject(hhandle, INFINITE);
	if(m_bDuringLock_saleitemtemp)
		CUniTrace::DumpMiniDump("�Ѿ�lock�˲��ó��ִ���� ");//�����Ϣ�ǳ���Ӧ�ó���
	m_bDuringLock_saleitemtemp=true;
}
void CSaleImpl::unlock_saleitem_temp()
{	return;//aori add proj 2011-8-3-1 ���۲�ƽ 2011-10-31 15:42:43 �޸��߼� �ƹ���ƽ���⣬ȡ����ƽ��־
	HANDLE hhandle=CreateMutex(NULL,0,"bDuringLock_saleitemtemp");
	WaitForSingleObject(hhandle, INFINITE);
	if(!m_bDuringLock_saleitemtemp)
		CUniTrace::DumpMiniDump("�Ѿ������˲��ó��ִ����" );//�����Ϣ�ǳ���Ӧ�ó���
	m_bDuringLock_saleitemtemp=false;
	ReleaseMutex(hhandle);
	ReleaseMutex(hhandle);
}
void CSaleImpl::check_lock_saleitem_temp()
{	return;//aori add proj 2011-8-3-1 ���۲�ƽ 2011-10-31 15:42:43 �޸��߼� �ƹ���ƽ���⣬ȡ����ƽ��־
	HANDLE hhandle=CreateMutex(NULL,0,"bDuringLock_saleitemtemp");
	int state=WaitForSingleObject(hhandle, 0);	int eee=0;
	switch(state)
	{
		case WAIT_FAILED:
			eee=GetLastError();
			CUniTrace::Trace(eee,"proj 2011-8-3-1 ���۲�ƽ!!!check_lock_saleitem_temp  waitforsingle failed");
			break;
		case WAIT_ABANDONED:
			CUniTrace::Trace("proj 2011-8-3-1 ���۲�ƽ!!!check_lock_saleitem_temp  wait abandoned");
			break;
		case WAIT_TIMEOUT:
			CUniTrace::DumpMiniDump("proj 2011-8-3-1 ���۲�ƽ ȷʵ���Զ��߳��ڸ���temp��");
			break;
		case WAIT_OBJECT_0:
			if(m_bDuringLock_saleitemtemp)
				CUniTrace::DumpMiniDump("proj 2011-8-3-1 ���۲�ƽ�ڲ��ø�temp���ʱ�����");
			ReleaseMutex(hhandle);
			break;
	}
}

void CSaleImpl::GeneralYinHuaInfo() //aori add 2011-8-30 10:13:21 proj2011-8-30-1 2011-8-30 10:13:21 ӡ������
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
	CDataBaseManager::closeConnection(pDB);//aori add debug:2011-10-6 11:23:54 proj 2011-10-6-1 2011-10-6 11:24:01 bug: �°汾��Ƶ�����ֳ�������������Ӧ�������
}

//<--##������ýӿ�##
//�ύ����
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
		msg.Format("�����ύ�ɹ�!\n%s",rtnMsg);
		AfxMessageBox(msg);
	}
	else*/
	if(rtn==SV_CALL_RETURN_TIMEOUT)
	{
		msg.Format("�����ύ��ʱ,δ����ȷ����Ϣ!\n%s\n���ͷ��绰ȷ�Ϸ���ɹ�!",rtnMsg);
		AfxMessageBox(msg);
	}
	else
	if(rtn==SV_CALL_RETURN_FAIL)
	{
		//�в���ȡ����֧����ʽ��
		if( !m_PayImpl.CanAllPaymentBeDel()  ) 
		{
			msg.Format("�ύ����ʧ��!\n%s\n����ʹ���˲���ȡ����֧����ʽ,�޷�ȡ���ñʽ���.\n����ϵ�ͷ����д���!",rtnMsg);
			AfxMessageBox(msg);
		}
		else
		{
			msg.Format("�ύ����ʧ��!\n%s",rtnMsg);
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

	//����
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
			cardfaceno=cardinput.Left(indexs);//��Ա��,ȥ��=����Ķ���
		}
		else
		{
			cardfaceno = cardinput.Left(18);//ȥǰ18λ
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

//��ѯ��Ա
bool CSaleImpl::QueryMember(bool *pDiffMember)
{
	m_MemberImpl.m_bMemberCardCancel = false;
	bool success=  m_MemberImpl.QueryMember(pDiffMember);
	m_bSaleCardCancel = m_MemberImpl.m_bMemberCardCancel;
	if(success && *pDiffMember)
	{
		//�洢����
		CPosClientDB* pDB = CDataBaseManager::getConnection(true);
		if ( !pDB ) {
			CUniTrace::Trace("CMemberImpl::QueryMember() �洢��Ա���ţ��޷���ȡ�������ݿ�����!");
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
	bool diffMember = (strcmp(m_MemberImpl.m_Member.szInputNO,strMemberNO)!=0); //��ͬ��Ա
	*pDiffMember=diffMember;

	//��ѯ����Ϣ
	bool success = false;
	if (GetSalePos()->GetParams()->m_nUseRemoteMemScore == VIPCARD_QUERYMODE_MEMBERPLATFORM )
		success = m_MemberImpl.QueryMember(strMemberNO,VIPCARD_NOTYPE_ID); 
	else
		success = m_MemberImpl.QueryMember(strMemberNO,VIPCARD_NOTYPE_FACENO); 
	if(success)
	{
		CString msg;
		msg.Format("��Ա����ѯ�ɹ� �����:%s ",strMemberNO);
		CUniTrace::Trace(msg); 
	}

	if(success && *pDiffMember)
	{
		//�洢����
		CPosClientDB* pDB = CDataBaseManager::getConnection(true);
		if ( !pDB ) {
			CUniTrace::Trace("CMemberImpl::QueryMember() �洢��Ա���ţ��޷���ȡ�������ݿ�����!");
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
//�����Ա��Ϣ
//void CSaleImpl::ClearMember()
//{
//	m_MemberImpl.Clear(); 
//}
//��ȡ��Ա��Ϣ
TMember CSaleImpl::GetMember()
{
	return m_MemberImpl.m_Member;
}

//��ѯ����Ӧ�����(�����ݿ�Sale_temp��)
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

//��ȡ������Ӫ����&���۽��
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

//��ӡ������Ӫ�������۽��
void CSaleImpl::PrintPoolClothesMerch(vector<SaleMerchInfo> *pVecSaleMerch)
{
	if (!GetSalePos()->GetParams()->m_bPoolClothesMerch) 
		return;

	map<CString, double> mapPoolClothesMerch;
	map<CString, double>::iterator itr;

	GetPoolClothesMerch(pVecSaleMerch,&mapPoolClothesMerch);
	GetSalePos()->GetPrintInterface()->nPrintHorLine();
	sprintf(m_szBuff, "��Ӫ������ϼ�:");
	GetSalePos()->GetPrintInterface()->nPrintLine(m_szBuff);	
 
	for (itr = mapPoolClothesMerch.begin(); itr != mapPoolClothesMerch.end(); itr++)
	{
		sprintf(m_szBuff,"%s                %.2f", itr->first, itr->second);
		GetSalePos()->GetPrintInterface()->nPrintLine(m_szBuff);
	}

	GetSalePos()->GetPrintInterface()->nPrintHorLine();
 
}

//��ȡ������Ʒ��Ӫ����SKU
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
// �� �� ����QueryEorderMemberCode
// ������������ѯ���̵ĵ��̻�Ա��
// ���������
// ���������bool
// �������ڣ�2015-3-2 
// �޸����ڣ�
// ��  �ߣ� LiuBW
// ����˵����
//======================================================================
bool CSaleImpl::QueryEorderMemberCode(bool* pDiffMember)
{
	CString EOrderMemberCode = GetSalePos()->m_params.m_EOrderMemberCode;
	bool diffMember=false;
	diffMember = (strcmp(m_MemberImpl.m_Member.szInputNO,EOrderMemberCode.GetBuffer(0))!=0);//��ͬ��Ա
	EOrderMemberCode.ReleaseBuffer();
	*pDiffMember=diffMember;

	bool success=  m_MemberImpl.QueryMember(EOrderMemberCode.GetBuffer(0),VIPCARD_NOTYPE_FACENO);//��ѯ���̵��̽����Ա��
	EOrderMemberCode.ReleaseBuffer();
	if(success && *pDiffMember)
	{
		//�洢����
		CPosClientDB* pDB = CDataBaseManager::getConnection(true);
		if ( !pDB ) {
			CUniTrace::Trace("CMemberImpl::QueryMember() �洢��Ա���ţ��޷���ȡ�������ݿ�����!");
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

//�жϸñʽ����Ƿ�Ϊ���̼��
bool CSaleImpl::IsEOrder()
{
	if(m_EorderID!=""&&m_EMemberID=="")
		return true;
	else 
		return false;
}

//�жϸñʽ����Ƿ�Ϊ���̻�Ա����
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
			if (type == 1)//֧���󱣴�
				strSQL.Format("update sale%s set addcol8 = '%d'  " 
						" WHERE  saledt >= GETDATE()-2 AND SaleID = %d  ",
						saledt,status , GetSaleID());
			else
				strSQL.Format("update sale_temp set EMOrderStatus = %d  ",status );
			GetSalePos()->GetWriteLog()->WriteLog(strSQL,3);//��־
			pDB->Execute2(strSQL);
			m_EMOrderStatus=status;
			strBuf.Format("���µ��̶���״̬���ݳɹ���");
			GetSalePos()->GetWriteLog()->WriteLog(strSQL,3);//��־

		}catch(CException *e) {
			CUniTrace::Trace(*e);
			e->Delete();
			strBuf.Format("���µ��̶���״̬����ʧ�ܣ�");
			GetSalePos()->GetWriteLog()->WriteLog(strSQL,3);//��־

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
		if (type == 1)//֧���󱣴� 
			strSQL.Format("update sale%s set addcol6 = '%s'   " 
					" WHERE  saledt >= GETDATE()-2 AND SaleID = %d  ",
					saledt,m_EorderID.GetBuffer(0) , GetSaleID());
		else
			strSQL.Format("UPDATE Sale_Temp SET EOrderID = '%s' ",m_EorderID.GetBuffer(0));

		GetSalePos()->GetWriteLog()->WriteLog(strSQL,3);//��־
		pDB->Execute2(strSQL);
		strBuf.Format("���µ��̶���״̬���ݳɹ���");
		GetSalePos()->GetWriteLog()->WriteLog(strSQL,3);//��־

	}catch(CException *e) {
		CUniTrace::Trace(*e);
		e->Delete();
		strBuf.Format("���µ��̶���״̬����ʧ�ܣ�");
		GetSalePos()->GetWriteLog()->WriteLog(strSQL,3);//��־

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
//���������ı��
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

	//������е�merchid�Ƿ��ڱ��ؿ��д���
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
			CString strErr = strSQL + _T("�ڱ���salemerch��û���ҵ�����Ʒ");
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
				//���������sqlִ�н��Ϊ�գ�����ֱ�Ӹ�ֵ Modified by dandan.wu [2016-1-20]
				pRecord->GetFieldValue((short)0, var);
				
				if(var.m_dwType != DBVT_NULL && strlen(*var.m_pstring)>0)
				{
					strSKU = *var.m_pstring;
					int nDiscType = atoi(strDiscType);
					double dbTemp = 0.0;
					CString strTempAmt = _T("0.0");
					dbTemp = atof(strAddCol4)*dSaleQty;

					double dSaleAmt = (nDiscType == PROMO_STYLE_MANUALDISCOUNT)?atof(strAfterMarkDownTotAmt):dbTemp;

					//����2λС��,�������ھ��������зֲ��죬����ʧ�ܣ����¶��˲�ƽ [dandan.wu 2016-8-3]	
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
					CString strErr = strSQL + _T("���ؿ�ֵ,����Ч��Ʒ��");
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

//���ݻ�Ա�Ų�ѯ����
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

//��Ա�ۿ�
void CSaleImpl::SetMemberPrice()
{
	double fDiscountRate = 1.0/*m_MemberImpl.m_Member.dMemberDiscountRate*/;//��Ա�ۿ���
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
//���ݶ����ŵõ�posID saleID saleDT
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
		strPrompt.Format("���ݶ����ŵõ�����Ϣ:%s",strInfo);
		CUniTrace::Trace(strPrompt);
		if(strInfo.IsEmpty())
			return FALSE;
		
		CMarkup xml = (CMarkup)strInfo;
		nPosID = atoi(GetSingleParamValue(xml, "PosID"));
		nSaleID = atoi(GetSingleParamValue(xml, "SaleID"));
		strSaleDT = GetSingleParamValue(xml, "SaleDT");
		strPrompt.Format("���ݶ����ŵõ���posID:%d  saleID:%d  saleDT:%s", nPosID, nSaleID, strSaleDT);
		CUniTrace::Trace(strPrompt);

		return TRUE;
	}
	AfxMessageBox("���ݶ����ŵò���������Ϣ!");
	CUniTrace::Trace("���ݶ����ŵò���������Ϣ");
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

//markdown���Ƿ���Ҫ�¼�һ��
int CSaleImpl::AddItemForMarkDown(double dCount, int nSID)
{
	if ( nSID >= m_vecSaleMerch.size() || nSID < 0 ) 
		return nSID;
	SaleMerchInfo *merchInfo = &m_vecSaleMerch[nSID];

	//��������fSaleQuantity�� [dandan.wu 2016-4-26]
	if ( merchInfo->fSaleQuantity/*nSaleCount*/ == 0 )
		return nSID;
	if( dCount < merchInfo->fSaleQuantity/*nSaleCount*/)
	{
		//������Ʒ�������� [dandan.wu 2016-4-26]
		if ( merchInfo->nMerchType == OPEN_STOCK_2 || merchInfo->nMerchType == OPEN_STOCK_5 )
		{
			merchInfo->nSaleCount = merchInfo->nSaleCount;
		}
		else
		{
			merchInfo->nSaleCount -=  (int)dCount;
		}	
		merchInfo->fSaleQuantity -= dCount;

		//���¼���fSaleAmt
		if(merchInfo->IncludeSKU>1)
			merchInfo->fSaleAmt = merchInfo->fActualBoxPrice * merchInfo->fSaleQuantity/*nSaleCount*//merchInfo->IncludeSKU;			
		else
			merchInfo->fSaleAmt = merchInfo->fActualPrice * merchInfo->fSaleQuantity/*nSaleCount*/;			
		merchInfo->fSaleAmt_BeforePromotion = merchInfo->fSaleAmt; 

		SaleMerchInfo newMerchInfo = m_vecSaleMerch[nSID];

		//������Ʒ����һ�У�����Ϊ1 [dandan.wu 2016-4-26]
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

		//���¼���fSaleAmt
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
	
	//��ӡ��ϸ��
	if(!GetSalePos()->GetPrintInterface()->m_bValid)
		return FALSE;
	BOOL bNeedSign = FALSE;
	
	CPrintInterface  *printer = GetSalePos()->GetPrintInterface();
	
	//��ӡ����
	if ( !bInvoicePrint)
	{
		GetSalePos()->GetPrintInterface()->nPrintHorLine();
	}
	
	const TPaymentStyle *ps = NULL;


	
	//��ӡ����ͷ��Ϣ
	if (bIsEndSign)
	{
		//��ֽ
		GetSalePos()->GetPrintInterface()->nEndPrint_m();
		GetSalePos()->GetWriteLog()->WriteLog("CSaleImpl::PrintBillBootomSmall(),��ֽ",POSLOG_LEVEL1);

		sprintf(m_szBuff, "����:%04d-%02d-%02d %02d:%02d ����:%03d", 
			m_stSaleDT.wYear, m_stSaleDT.wMonth, m_stSaleDT.wDay, m_stSaleDT.wHour, m_stSaleDT.wMinute, m_pParam->m_nPOSID);
		printer->nPrintLine(m_szBuff);
		
		sprintf(m_szBuff, "����:%06d ����Ա:%s", 
			m_nSaleID,GetSalePos()->GetCasher()->GetUserCode());
		printer->nPrintLine(m_szBuff);
	}
	
	//��ӡ֧����ʽ�����
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
					sprintf(m_szBuff,"֧����:%12.2f",payInfoItem->fPaymentAmt);
				}else{
					sprintf(m_szBuff,"΢��:%12.2f",payInfoItem->fPaymentAmt);
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
					sprintf(m_szBuff,"�̻��Żݽ��:%s",strOrgDiscount);
					printer->nPrintLine( m_szBuff);

					CString strOtherDiscount = "";
					strOtherDiscount = strDiscount.Mid(12,12);
					strOtherDiscount = FormatMiyaDiscount(strOtherDiscount);
					sprintf(m_szBuff,"�����Żݽ��:%s",strOtherDiscount);
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
						sprintf(m_szBuff,"�̻�������:%s",strUniqueOrderID);
						printer->nPrintLine( m_szBuff);
					}	
					if (ExtraInfo.GetLength() >= 31)
					{
						CString strSysPayNum = "";
						strSysPayNum = ExtraInfo.Mid(3,28);
						sprintf(m_szBuff,"֧��ƽ̨��:%s",strSysPayNum);
						printer->nPrintLine( m_szBuff);
					}
					if (ExtraInfo.GetLength() >= 42)
					{
						CString strCustomerID = "";
						strCustomerID = ExtraInfo.Mid(31,11);
						sprintf(m_szBuff,"֧���˺�:%s",strCustomerID);
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
		sprintf(m_szBuff, "%s%s", "��Աǩ��:","....................");
		printer->nPrintLine(m_szBuff);
		printer->nPrintLine(" ");
		sprintf(m_szBuff, "%s%s", "�˿�ǩ��:","....................");
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

	//��ӡ��ֵ����ϸ��
	if(!GetSalePos()->GetPrintInterface()->m_bValid)
		return FALSE;
	BOOL bNeedSign = FALSE;

	CPrintInterface  *printer = GetSalePos()->GetPrintInterface();
	//int i=0,nMaxLen = GetSalePos()->GetParams()->m_nMaxPrintLineLen;
	//if ( GetSalePos()->GetParams()->m_nPaperType == 1 )
	//	nMaxLen = 32;	//��ӡ����
	//GetPrintChars(m_szBuff, nMaxLen, nMaxLen, true);
	//printer->nPrintLine( m_szBuff );

	//��ӡ����
	if ( !bInvoicePrint)
	{
		GetSalePos()->GetPrintInterface()->nPrintHorLine();
	}

	const TPaymentStyle *ps = NULL;


	//��ӡ����ͷ��Ϣ
	if (bIsEndSign)
	{
		
		GetSalePos()->GetPrintInterface()->nEndPrint_m();
		GetSalePos()->GetWriteLog()->WriteLog("CSaleImpl::PrintBillBootomSmall(),��ֽ",POSLOG_LEVEL1);

		sprintf(m_szBuff, "����:%04d-%02d-%02d %02d:%02d ����:%03d", 
			m_stSaleDT.wYear, m_stSaleDT.wMonth, m_stSaleDT.wDay, m_stSaleDT.wHour, m_stSaleDT.wMinute, m_pParam->m_nPOSID);
		printer->nPrintLine(m_szBuff);

		sprintf(m_szBuff, "����:%06d ����Ա:%s", 
			m_nSaleID,GetSalePos()->GetCasher()->GetUserCode());
		printer->nPrintLine(m_szBuff);
	}

	//��ӡ֧����ʽ�����
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
				sprintf(m_szBuff,"��ֵ��:%12.2f",payInfoItem->fPaymentAmt);
				printer->nPrintLine( m_szBuff);
				sprintf(m_szBuff,"����:%s",payInfoItem->szPaymentNum);
				printer->nPrintLine(m_szBuff);
				sprintf(m_szBuff,"�ֿ���:%s",payInfoItem->szExtraInfo);
				printer->nPrintLine(m_szBuff);
				sprintf(m_szBuff,"���:%14.2f",payInfoItem->fRemainAmt);
				printer->nPrintLine(m_szBuff);
			}
		}
	}

	if (bNeedSign&&!bInvoicePrint)
	{
		printer->nPrintLine(" ");
		sprintf(m_szBuff, "%s%s", "��Աǩ��:","....................");
		printer->nPrintLine(m_szBuff);
		printer->nPrintLine(" ");
		sprintf(m_szBuff, "%s%s", "�˿�ǩ��:","....................");
		printer->nPrintLine(m_szBuff);
		printer->nPrintLine(" ");
		printer->nPrintLine(" ");
	}
	return bNeedSign;
}

void CSaleImpl::InvoicePrintWholeSale(bool& bInvoiceSegmentUseUp)
{
	//��ӡ�����������˳�
	if (!GetSalePos()->GetPrintInterface()->m_bValid) 
	{
		GetSalePos()->GetWriteLog()->WriteLog("END CSaleImpl::InvoicePrintWholeSale PrintInterface::m_bValid��ֹ��ӡ",4,POSLOG_LEVEL1); 
		return;
	} 
	
	if ( !GetSalePos()->m_params.m_bAllowInvoicePrint )
	{
		GetSalePos()->GetWriteLog()->WriteLog("END CSaleImpl::InvoicePrintWholeSale ��ֹ��Ʊ��ӡ",4,POSLOG_LEVEL1); 
		return;
	}

	CUniTrace::Trace("********************************��ʼ��ӡ���۷�Ʊ********************************");
	char szBuff[128] = {0};
	sprintf(szBuff,"(��������ӡ�Ĵ�ֵ����֧����/΢�ţ����ۣ���Ʊ������:%d,��ҳ��:%d",m_nInvoiceTotalPrintLineSale,m_nInvoiceTotalPageSale);
	CUniTrace::Trace(szBuff);

	sprintf(szBuff,"������ӡ��ֵ������Ʊ������:%d,��ҳ��:%d",m_nInvoiceTotalPrintLinePrepaid,m_nInvoiceTotalPagePrepaid);
	CUniTrace::Trace(szBuff);

	sprintf(szBuff,"֧����/΢��ֱ������Ʊ������:%d,��ҳ��:%d",m_nInvoiceTotalPrintLineAPay,m_nInvoiceTotalPageAPay);
	CUniTrace::Trace(szBuff);

	//������һ�ʽ��׵ĵ�һ�ŷ�Ʊ��=���һ�ʽ��ף����ۡ���ֵ����ֵ/���֣���������һ�ŷ�Ʊ��+1
	InvoiceUpdateInvoiceNO(atoi(m_strInvoiceNoFirst)+m_nInvoiceTotalPage);

	//��Ʊͷ����
	InvoicePrintHead(m_strInvoiceNoFirst,m_nInvoiceTotalPageSale);

	//�ֻ���Ʒ
	printBillItemInStock(m_vecSaleMerch);

	//������Ʒ
	printBillItemInOrder(m_vecSaleMerch);

	//֧����ʽ������
	InvoicePrintPayment();

	//��Ʊ��ӡ����֧����ʽ���д�ֵ������ӡ���׺󣬵�����ӡ
	InvoicePrintPrepaidCard();

	//��֧����/΢��ֱ����������ӡ
	m_InvoiceImpl.InvoicePrintAPay(m_vecPayItemAPay);

	InvoiceRest();

	//�жϷ�Ʊ���Ƿ����꣬�����꣬��ʾ�û����µķ�Ʊ��
	if ( m_eInvoiceRemain == INVOICE_REMAIN_EQUAL )
	{
		bInvoiceSegmentUseUp = true;

		CString strMsg = _T("��Ʊ�����꣬������µķ�Ʊ��");
		
		//��ʾ�û�����Ʊ��
		MessageBox(NULL,strMsg, "��ʾ", MB_OK | MB_ICONWARNING);		
		CUniTrace::Trace(strMsg);
		
		//�ൺ��Ʊ���һ�Ŵ�ӡ������Ϣ
		if ( GetSalePos()->m_params.m_strInvoiceDistrict == INVOICE_DISTRICT_QD )
		{
			InvoicePrintSummaryInfo();
		}
		
		//���¾ɷ�Ʊ��ķ�Ʊ��
		InvoiceUpdateInvoiceNO(atoi(m_strInvoiceEndNo)+1);
	}

	CUniTrace::Trace("********************************������ӡ���۷�Ʊ********************************");
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
	
	//����/ʱ��
	stPrintHead.stSaleDT = m_stSaleDT;	

	//���һ�ʱ����������ˮ��
	stPrintHead.nSaleID = m_nSaleID;

	//��Ա����
	memcpy(stPrintHead.szMemberCode,GetMember().szMemberCode,strlen(GetMember().szMemberCode));
	
	//���λ����Ա���ƣ�
	memcpy(stPrintHead.szMemberName,GetMember().szMemberName,strlen(GetMember().szMemberName));

	//��Ʊ���ڳ��м��,���ൺ��QD ���ݣ�SZ
	sprintf(stPrintHead.szInvoiceDistrict,"%s",strDistrict);

	//��Ʊ����
	sprintf(stPrintHead.szInvoiceCode,"%s",m_strInvoiceCode);

	//��Ʊ��
	sprintf(stPrintHead.szInvoiceNo,"%s",strInvoiceNo);

	//�˱ʽ�����Ҫ��ӡ�ķ�Ʊ��ҳ��
	 stPrintHead.nInvoiceTotalPrintLineNormal = m_nInvoiceTotalPrintLineSale;
	 stPrintHead.nInvoiceTotalPageNormal = nTotalPage;

	 stPrintHead.nInvoiceTotalPrintLinePrepaid = m_nInvoiceTotalPrintLinePrepaid;
	 stPrintHead.nInvoiceTotalPagePrepaid = m_nInvoiceTotalPagePrepaid;

	 stPrintHead.nInvoiceTotalPrintLineAPay = m_nInvoiceTotalPrintLineAPay;
	 stPrintHead.nInvoiceTotalPageAPay = m_nInvoiceTotalPageAPay;

	//�ۿ��ܽ��
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
	double dbInvoiceAmt = 0.0;//ʵ��֧�����

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
					//֧����������
					strcpy(pPaymentItem.szDescription,ps->szDescription);
					
					//֧�����	
					pPaymentItem.fPaymentAmt = payInfoItem->fPaymentAmt;
					
					//����
					strcpy(pPaymentItem.szPaymentNum,payInfoItem->szPaymentNum);
					
					//��Ϊ���ڸ����Ҫ��ӡ����ID���������������� [dandan.wu 2016-3-28]
					if ( payInfoItem->nPSID == ::GetSalePos()->GetParams()->m_nInstallmentPSID )
					{
						pPaymentItem.nLoanID = payInfoItem->nLoanID;
						pPaymentItem.nLoanPeriod = payInfoItem->nLoanPeriod;
						if ( strlen(payInfoItem->szLoanType) == 0 )
						{
							CUniTrace::Trace("���ۣ���������Ϊ��");
						}
						else
						{
							strcpy(pPaymentItem.szLoanType,payInfoItem->szLoanType);
						}

						if ( strlen(payInfoItem->szLoanID) == 0 )
						{
							CUniTrace::Trace("���ۣ�����IDΪ��");
						}
						else
						{
							strcpy(pPaymentItem.szLoanID,payInfoItem->szLoanID);
						}
					}					
					
					//aori add ���п���ӡ���ε�����5������12λ
					HideBankCardNum(pay, pPaymentItem);
					
					if (pPaymentItem.fPaymentAmt >= 0 )
						GetSalePos()->GetPrintInterface()->nPrintPayment(&pPaymentItem);//��ӡ֧��							
				}
			}
		}
		else
		{
			if ( NULL != ps )
			{
				strcpy(pPaymentItem.szDescription,ps->szDescription);
			}

			//���
			pPaymentItem.fPaymentAmt = pay->fPayAmt;
			
			if ( NULL != ps && !ps->bNeedReadCard) //��IC����paynum
				strcpy(pPaymentItem.szPaymentNum,pay->szPayNum);
			
			if (pPaymentItem.fPaymentAmt > 0 )
				GetSalePos()->GetPrintInterface()->nPrintPayment(&pPaymentItem);//��ӡ֧��
		}

		if ( NULL != ps && ps->bForChange)
		{
			forchange_amt -= pay->fPayAmt;
		}

	}

	//��ӡ����
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

	int nRemain = 0;				//����
	int nRemainLine = 0;			//һҳ��ʣ����ܴ�ӡ����
	int nMaxLineForOnePage = 0;		//һҳ���ܴ�ӡ���������
	vector<CString> vecStrOrderNo;	//������
	vector<CString>::iterator itOrderNo;	
	const TPaymentStyle *ps = NULL;

	nMaxLineForOnePage = GetSalePos()->m_params.m_nInvoicePageMaxLine;

	//����������ҳ������0
	InvoiceRest();

	//��÷�Ʊ��
	GetInvocieNo(m_strInvoiceNoFirst);

	strDistrict = GetSalePos()->m_params.m_strInvoiceDistrict;
	
	//ͷ������ͳ��
	nAddLine = m_InvoiceImpl.InvoiceGetHeadLine();
	m_nInvoiceHeadLine = nAddLine;
	InvoicePaging(nAddLine);

	int nStockItem = 0,nOrderItem = 0;
	for (merchInfo = m_vecSaleMerch.begin(); merchInfo != m_vecSaleMerch.end(); ++merchInfo) 
	{
		//��ȡ����Ʒ����	
		if (merchInfo->nMerchState == RS_CANCEL)
		{
			continue;
		}
		
		//ͳ���ֻ���Ʒ
		if ( merchInfo->nItemType == ITEM_IN_STOCK )
		{
			nStockItem++;
		}
		//������ж�����
		else if ( merchInfo->nItemType == ITEM_IN_ORDER && !merchInfo->strOrderNo.IsEmpty() )
		{
			nOrderItem++;
			vecStrOrderNo.push_back(merchInfo->strOrderNo);	
		}
	}

	//��Ʒ��ʼ�� = ������+1
	if ( (nStockItem + nOrderItem) != 0 )
	{
		m_nInvoiceSaleMerchLineStart = m_nInvoiceTotalPrintLineSale+1;
	}

	//��ӡ�ֻ���Ʒ
	if ( nStockItem >= 1 )
	{
		//���⣺�ֻ���Ʒ
		InvoicePaging(1);

		//�ֻ���ϸ
		for (merchInfo = m_vecSaleMerch.begin(); merchInfo != m_vecSaleMerch.end(); ++merchInfo) 
		{
			//��ȡ����Ʒ����	
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

	//������ȥ��
	if ( vecStrOrderNo.size() > 1 )
	{
		sort(vecStrOrderNo.begin(), vecStrOrderNo.end()); 
		
		itOrderNo = unique(vecStrOrderNo.begin(),vecStrOrderNo.end());	
		vecStrOrderNo.erase( itOrderNo, vecStrOrderNo.end());
	}

	for ( itOrderNo = vecStrOrderNo.begin();itOrderNo != vecStrOrderNo.end(); ++itOrderNo )
	{
		//��ӡ������
		InvoicePaging(1);
		
		//��ӡ������ϸ
		for (merchInfo = m_vecSaleMerch.begin(); merchInfo != m_vecSaleMerch.end(); ++merchInfo) 
		{
			if ( merchInfo->nMerchState != RS_CANCEL 
				&& merchInfo->nItemType == ITEM_IN_ORDER 
				&& itOrderNo->Compare(merchInfo->strOrderNo) == 0 )
			{
				//��ȡ����Ʒ����	
				if (merchInfo->nMerchState == RS_CANCEL)
				{
					continue;
				}

				nAddLine = InvoiceGetSaleMerchLine(merchInfo->fManualDiscount);
				InvoicePaging(nAddLine);
			}
		}
	}
	
	//��Ʒ���һ��
	m_nInvoiceSaleMerchLineEnd = m_nInvoiceTotalPrintLineSale;

	//�ڴ�ӡ֧��֮ǰ������ҳ����Ʒ������Ʒ�����һ���뵱ǰ����ͬһҳ����ӡС�� [dandan.wu 2016-11-15]
	int nCurLine = m_nInvoiceTotalPrintLineSale+1;
	int nPage = m_InvoiceImpl.InvoiceGetPageFromLine(nCurLine);
	int nSaleMerchPageEnd = m_InvoiceImpl.InvoiceGetPageFromLine(m_nInvoiceSaleMerchLineEnd);	
	if ( nPage == nSaleMerchPageEnd )
	{
		InvoicePaging(INVOICE_LINE_SUBTATAL);
	}

	//֧������
	m_nInvoicePaymentLineStart = m_nInvoiceTotalPrintLineSale+1;
	for ( vector<PayInfo>::const_iterator pay = m_PayImpl.m_vecPay.begin(); pay != m_PayImpl.m_vecPay.end(); ++pay) 
	{
		if ( pay->nPSID == GetSalePos()->m_params.m_nSavingCardPSID)
		{
			m_bHasPrepaidCardPS = true;
		}

		ps = m_PayImpl.GetPaymentStyle(pay->nPSID);

		if ( NULL != ps && ps->bPrintOnVoucher)//����СƱ�ϴ�ӡ
		{	
			if ( ps->bNeedPayNO )
			{
				for ( vector<PayInfoItem>::iterator payInfoItem = m_PayImpl.m_vecPayItem.begin(); 
					payInfoItem != m_PayImpl.m_vecPayItem.end(); ++payInfoItem) 
				{
					if ( payInfoItem->nPSID == pay->nPSID ) 
					{
						//���ڸ���
						if ( pay->nPSID == GetSalePos()->m_params.m_nInstallmentPSID)
						{
							nAddLine = INVOICE_LINE_PS_INSTALLMENT;
						}
						//֧����/΢���л���ʱ����ӡ���ţ����ߴ�ӡ
						//ֱ������Ҫ��ӡ���� [dandan.wu 2017-10-19]
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

						//�����ŵ�֧����ʽ	
						InvoicePaging(nAddLine);
					}					
				}		
			}	
			else
			{
				nAddLine = 1;

				//�������ŵ�֧����ʽ	
				InvoicePaging(nAddLine);
			}
		}	
	}
	m_nInvoicePaymentLineEnd = m_nInvoiceTotalPrintLineSale;

	//����
	InvoicePaging(1);

	//��Ա����
	if ( strlen(GetMember().szMemberCode) > 0  )
	{
		InvoicePaging(1);
	}
	
	//���׺�
	InvoicePaging(1);

	//+�ܼ�3��
	//�����ൺ��Ʊ����Ϊ��ʽ��ӡ����һ�ŷ�Ʊ��������ӡ20�У����һ�ŷ�Ʊʵ���ϴ�ӡ��23��
	//if ( strDistrict != INVOICE_DISTRICT_QD )
	{	
		m_nInvoiceAmtTotalLineStart = m_nInvoiceTotalPrintLineSale+1;
		InvoicePaging(INVOICE_LINE_TOTAL);
		m_nInvoiceAmtTotalLineEnd = m_nInvoiceTotalPrintLineSale;
	}

	//���д�ֵ������֧����ʽ����ӡ��Ϻ���ֽ���ٵ�����ӡһ�Ÿ�����Ա���棬��������
	InvoiceGetTotalPageAndLinePrepaid();

	//֧����/΢��ֱ����������ӡ
	InvoiceGetTotalPageAndLineAPay();

	//����һ��ʼ����������ֵ������Ҫ�õ�
	m_nInvoiceTotalPage = m_nInvoiceTotalPageSale+m_nInvoiceTotalPagePrepaid+m_nInvoiceTotalPageAPay;
	
	//Ϊÿ����Ʒ��ֵ�þ�Ʊ��ĩβ��
	CString strInvoiceNoTemp = _T("");
	int nInvoiceNoTemp = 0; 
	for ( merchInfo = m_vecSaleMerch.begin(); merchInfo != m_vecSaleMerch.end(); ++merchInfo )
	{
		//��ȡ����Ʒ����	
		if (merchInfo->nMerchState == RS_CANCEL)
		{
			continue;
		}	
		
		nInvoiceNoTemp = atoi(m_strInvoiceNoFirst) + m_nInvoiceTotalPage-1;
		strInvoiceNoTemp.Format("%08d",nInvoiceNoTemp);//��Ʊ��8λ�����㲹0

		//�ñʽ��׵����һ�ŷ�Ʊ��
		merchInfo->strInvoiceEndNo = strInvoiceNoTemp;

		//���±�SaleItem_Temp��InvoiceEndNo
		m_InvoiceImpl.UpdateSaleItemInvoiceNo(strInvoiceNoTemp,merchInfo->szPLUInput);
	}
}

int CSaleImpl::InvoiceGetSaleMerchLine(double dbfManualDiscount)
{
	int nAddLine = 0;

	//��Ʒ��MarkDown����ӡ3�У���Ʒ��MarkUp,��ӡ2�� [dandan.wu 2017-11-8]
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
	int nRemain = 0;				//����
	int nRemainLine = 0;			//һҳ��ʣ����ܴ�ӡ����
	int nMaxLineForOnePage = 0;		//һҳ���ܴ�ӡ���������	
	int nRealNeedLine = 0;			//ʵ����Ҫ��ӡ������������Ҫ����С��
	bool bPrintSubTotal = false;	//��Ҫ��ӡС��
	int nBlankLine = 0;
	int nPage = 0;
	int nSaleMerchPageEnd = 0;

 	nMaxLineForOnePage = GetSalePos()->m_params.m_nInvoicePageMaxLine;
	nRemain = m_nInvoiceTotalPrintLineSale % nMaxLineForOnePage;
	nPage = m_InvoiceImpl.InvoiceGetPageFromLine(m_nInvoiceTotalPrintLineSale);
	nSaleMerchPageEnd = m_InvoiceImpl.InvoiceGetPageFromLine(m_nInvoiceSaleMerchLineEnd);
	
	//����ǰ�����պ�������������ҳ����ӡͷ��
	if ( m_nInvoiceTotalPrintLineSale > 1 && nRemain == 0 )
	{
		//ͷ����Ϣ
 		m_nInvoiceTotalPrintLineSale += m_nInvoiceHeadLine;

		//��������
		nRemain = m_nInvoiceTotalPrintLineSale % nMaxLineForOnePage;
	}

	//һҳ��ʣ����ܴ�ӡ����
	nRemainLine = nMaxLineForOnePage - nRemain;

	//��Ҫ��ӡ������
	nRealNeedLine += nAddLine;

	//��Ʒ��û����ӡ�꣬Ҫ��ӡС��
	if ( m_nInvoiceSaleMerchLineEnd == 0 )
	{
		bPrintSubTotal = true;
		nRealNeedLine += INVOICE_LINE_SUBTATAL;
	}
	else
	{
		bPrintSubTotal = false;
	}

	//ʣ������>��Ҫ��ӡ�����������û�ҳ
	if ( nRemainLine > nRealNeedLine )
	{
		//��ӡ����
		m_nInvoiceTotalPrintLineSale += nAddLine;
	}
	//ʣ������=��Ҫ��ӡ������,���û�ҳ
	else if ( nRemainLine == nRealNeedLine )
	{	
		//��ӡ����
		m_nInvoiceTotalPrintLineSale += nAddLine;

		//��ӡС��
		if ( bPrintSubTotal )
		{
			m_nInvoiceTotalPrintLineSale += INVOICE_LINE_SUBTATAL;
		}
	}
	//ʣ������<��Ҫ��ӡ��������˵����Ҫ��ҳ��
	else
	{	
		//�ȴ�ӡ����
		if ( bPrintSubTotal )
		{
			nBlankLine = nRemainLine - INVOICE_LINE_SUBTATAL;
		}
		else
		{
			nBlankLine = nRemainLine;
		}
		m_nInvoiceTotalPrintLineSale += nBlankLine;

		//��ӡС��
		if ( bPrintSubTotal )
		{
			m_nInvoiceTotalPrintLineSale += INVOICE_LINE_SUBTATAL;
		}

		//��ӡ��Ʊͷ
 		m_nInvoiceTotalPrintLineSale += m_nInvoiceHeadLine;

		//��ӡ����
		m_nInvoiceTotalPrintLineSale += nAddLine;
	}	

	//����ҳ��
	m_nInvoiceTotalPageSale = m_InvoiceImpl.InvoiceGetPageFromLine(m_nInvoiceTotalPrintLineSale);
}

double CSaleImpl::InvoiceGetTotalDisAmt()
{
	double dbTotalDisAmt = 0.0;

	for ( vector<SaleMerchInfo>::iterator merchInfo = m_vecSaleMerch.begin(); merchInfo != m_vecSaleMerch.end(); ++merchInfo )
	{
		if ( merchInfo->nMerchState != RS_CANCEL ) 
		{
			dbTotalDisAmt += merchInfo->dCondPromDiscount;//��������

			//MarkUp������ [dandan.wu 2017-11-8]
			if ( merchInfo->fManualDiscount > 0 )
			{
				dbTotalDisAmt += merchInfo->fManualDiscount;
			}
			  //MarkDown/MarkUp
			dbTotalDisAmt += merchInfo->dMemDiscAfterProm;//��Ա�ۿ�
		}
	}

	char szBuff[32] = {0};
	sprintf(szBuff,"��Ʊ�ۿ��ܽ�%.2f",dbTotalDisAmt);
	CUniTrace::Trace(szBuff);

	return dbTotalDisAmt;
}

void CSaleImpl::HandleInvoiceRemainPage(eInvoiceRemain& eType)
{
	eType = INVOICE_REMAIN_GREATER;

	//��ӡ�����������˳�
	if (!GetSalePos()->GetPrintInterface()->m_bValid) 
	{
		GetSalePos()->GetWriteLog()->WriteLog("END CSaleImpl::IsChangeInvoiceSegment PrintInterface::m_bValid��ֹ��ӡ",4,POSLOG_LEVEL1); 
		return;
	} 
	
	if ( !GetSalePos()->m_params.m_bAllowInvoicePrint )
	{
		GetSalePos()->GetWriteLog()->WriteLog("END CSaleImpl::IsChangeInvoiceSegment ��ֹ��Ʊ��ӡ",4,POSLOG_LEVEL1); 
		return;
	}

	//��÷�Ʊ��ҳ����������
	InvoiceGetTotalPageAndLine();

	m_InvoiceImpl.HandleInvoiceRemainPage(m_nSegmentID,m_strInvoiceCode,m_strInvoiceNoFirst,
		m_strInvoiceEndNo,m_nInvoiceTotalPage,eType);
	m_eInvoiceRemain = eType;

	//ʣ�෢Ʊ������
	if ( eType  == INVOICE_REMAIN_LESS )
	{
		if ( GetSalePos()->m_params.m_strInvoiceDistrict == INVOICE_DISTRICT_QD )
		{					
			//��ֽ�����һ�ŷ�Ʊ
			m_InvoiceImpl.FeedPaper(atoi(m_strInvoiceNoFirst),atoi(m_strInvoiceEndNo));

			//��ӡ������Ϣ
			InvoicePrintSummaryInfo();
		}
		else
		{
			//ʣ�෢Ʊȫ����ֽ
			m_InvoiceImpl.FeedPaper(atoi(m_strInvoiceNoFirst),atoi(m_strInvoiceEndNo)+1);
		}	

		//���¾ɷ�Ʊ��ķ�Ʊ��
		InvoiceUpdateInvoiceNO(atoi(m_strInvoiceEndNo)+1);

		//��ʾ�û�����Ʊ��
		CString strMsg = _T("ʣ�෢Ʊ�����Դ�ӡ���ν��ף�������µķ�Ʊ�����÷�Ʊ���\n��1�����°�����Ʊ������ӡ\n��2����ʣ�෢Ʊ��ȥ��̨����");
		MessageBox(NULL,strMsg, "��ʾ", MB_OK | MB_ICONWARNING);		
		CUniTrace::Trace(strMsg);
	}
}

void CSaleImpl::InvoiceRest()
{
	//����������ҳ������0
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
		CUniTrace::Trace("CSaleImpl::GetInvocieNo(),����Ϊ�գ���÷�Ʊ��ʧ�ܣ�����");
		return;
	}

	if ( !GetSalePos()->m_params.m_bAllowInvoicePrint )
	{
		CUniTrace::Trace("CSaleImpl::GetInvocieNo(),�Ƿ�Ʊ��ӡ����÷�Ʊ��ʧ�ܣ�����");
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
		
		strTemp += _T("��Ʊ���룺");
		strTemp += strInvoiceCode;
		strTemp += _T("��Ʊ��ʼ�ţ�");
		strTemp += strStartNo;
		strTemp += _T("��Ʊĩβ�ţ�");
		strTemp += strEndNo;
		CUniTrace::Trace(strTemp);
		
		//���¼��㷢Ʊ��ҳ��������������Ʒ��Ӧ�ķ�Ʊ��
		//InvoiceGetTotalPageAndLine();	

		//��÷�Ʊ��
		GetInvocieNo(m_strInvoiceNoFirst);
		
		//���·�Ʊ��
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
		//֧����������
		ps = m_PayImpl.GetPaymentStyle(nPrepaidPSID);
		if ( NULL != ps )
		{
			strcpy(pPaymentItem.szDescription,ps->szDescription);
		}
		
		//֧�����
		pPaymentItem.fPaymentAmt = payInfoItem->fPaymentAmt;
		
		//���
		pPaymentItem.fRemainAmt = payInfoItem->fRemainAmt;
		
		//����
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
	//��û�����Ϣ
	TInvoicePrintSummary stSummary;
	m_InvoiceImpl.LoadRemoteInvoiceSumInfo(m_nSegmentID,m_strInvoiceCode,m_strInvoiceStartNo,m_strInvoiceEndNo,stSummary);
	
	//��ӡͷ����
	InvoicePrintHead(m_strInvoiceEndNo,1);
	
	//��ӡ������Ϣ
	GetSalePos()->GetPrintInterface()->fnInvoicePrintSummary_m(&stSummary,PBID_INVOICE_BNQ);
}

void CSaleImpl::InvoiceUpdateInvoiceNO(const int& nInvoiceNo)
{
	CString strInvoiceNoTemp = _T("");

	strInvoiceNoTemp.Format("%08d",nInvoiceNo);//��Ʊ��8λ�����㲹0
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
		CUniTrace::Trace("û��֧������΢�ţ�ֱ����������Ҫ������ӡ");
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