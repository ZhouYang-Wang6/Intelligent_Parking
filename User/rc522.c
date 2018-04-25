#include "stm32f10x.h"
#include "rc522.h"

void delay_ns(u32 ns)
{
        u32 i;
        for(i=0; i<ns; i++)
        {
                __nop();
                __nop();
                __nop();
        }
}

u8 SPIWriteByte(u8 Byte)
{
        while((SPI1->SR&0X02)==0); //�ȴ���������
        SPI1->DR=Byte;          //����һ��byte
        while((SPI1->SR&0X01)==0); //�ȴ�������һ��byte
        return SPI1->DR;          //�����յ�������
}

//SPIx ��дһ���ֽ�
//TxData:Ҫд�����ֽ�
//����ֵ:��ȡ�����ֽ�
u8 SPI2_ReadWriteByte(u8 TxData)
{
        u8 retry=0;
        while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET) //����ָ����SPI��־λ��������:���ͻ����ձ�־λ
        {
                retry++;
                if(retry>200) return 0;
        }
        SPI_I2S_SendData(SPI1, TxData); //ͨ������SPIx����һ������
        retry=0;

        while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET) //����ָ����SPI��־λ��������:���ܻ����ǿձ�־λ
        {
                retry++;
                if(retry>200) return 0;
        }
        return SPI_I2S_ReceiveData(SPI1); //����ͨ��SPIx�������յ�����
}


//SPI �ٶ����ú���
//SpeedSet:
//SPI_BaudRatePrescaler_2   2��Ƶ
//SPI_BaudRatePrescaler_8   8��Ƶ
//SPI_BaudRatePrescaler_16  16��Ƶ
//SPI_BaudRatePrescaler_256 256��Ƶ

void SPI2_SetSpeed(u8 SPI_BaudRatePrescaler)
{
        assert_param(IS_SPI_BAUDRATE_PRESCALER(SPI_BaudRatePrescaler));
        SPI1->CR1&=0XFFC7;
        SPI1->CR1|=SPI_BaudRatePrescaler; //����SPI2�ٶ�
        SPI_Cmd(SPI1,ENABLE);

}




void SPI1_Init(void)
{
        GPIO_InitTypeDef GPIO_InitStructure;
        SPI_InitTypeDef SPI_InitStructure;
        RCC_APB2PeriphClockCmd( RCC_APB2Periph_GPIOA|RCC_APB2Periph_GPIOB, ENABLE );//PORTBʱ��ʹ��
        RCC_APB2PeriphClockCmd( RCC_APB2Periph_SPI1,  ENABLE );//SPI2ʱ��ʹ��

        GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0|GPIO_Pin_1; //IO-->PF0��PF1 �˿�����
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; //��������
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; //IO���ٶ�Ϊ50MHz
        GPIO_Init(GPIOB, &GPIO_InitStructure);     //�����趨������ʼ��PF0��PF1

        GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7;
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP; //PB13/14/15������������
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
        GPIO_Init(GPIOA, &GPIO_InitStructure);//��ʼ��GPIOB

        GPIO_SetBits(GPIOA,GPIO_Pin_5|GPIO_Pin_6|GPIO_Pin_7); //PB13/14/15����

        SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex; //����SPI��������˫��������ģʽ:SPI����Ϊ˫��˫��ȫ˫��
        SPI_InitStructure.SPI_Mode = SPI_Mode_Master; //����SPI����ģʽ:����Ϊ��SPI
        SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b; //����SPI�����ݴ�С:SPI���ͽ���8λ֡�ṹ
        SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low; //����ͬ��ʱ�ӵĿ���״̬Ϊ�͵�ƽ
        SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge; //����ͬ��ʱ�ӵĵ�һ�������أ��������½������ݱ�����
        SPI_InitStructure.SPI_NSS = SPI_NSS_Soft; //NSS�ź���Ӳ����NSS�ܽţ�����������ʹ��SSIλ������:�ڲ�NSS�ź���SSIλ����
        SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_256; //���岨����Ԥ��Ƶ��ֵ:������Ԥ��ƵֵΪ256
        SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB; //ָ�����ݴ�����MSBλ����LSBλ��ʼ:���ݴ�����MSBλ��ʼ
        SPI_InitStructure.SPI_CRCPolynomial = 7; //CRCֵ�����Ķ���ʽ
        SPI_Init(SPI1, &SPI_InitStructure); //����SPI_InitStruct��ָ���Ĳ�����ʼ������SPIx�Ĵ���

        SPI_Cmd(SPI1, ENABLE); //ʹ��SPI����
}

void InitRc522(void)
{
        SPI1_Init();
        PcdReset();
        PcdAntennaOff();
        delay_ms(2);
        PcdAntennaOn();
        M500PcdConfigISOType( 'A' );
}
void Reset_RC522(void)
{
        PcdReset();
        PcdAntennaOff();
        delay_ms(2);
        PcdAntennaOn();
}
/////////////////////////////////////////////////////////////////////
//��    �ܣ�Ѱ��
//����˵��: req_code[IN]:Ѱ����ʽ
//                0x52 = Ѱ��Ӧ�������з���14443A��׼�Ŀ�
//                0x26 = Ѱδ��������״̬�Ŀ�
//          pTagType[OUT]����Ƭ���ʹ���
//                0x4400 = Mifare_UltraLight
//                0x0400 = Mifare_One(S50)
//                0x0200 = Mifare_One(S70)
//                0x0800 = Mifare_Pro(X)
//                0x4403 = Mifare_DESFire
//��    ��: �ɹ�����MI_OK
/////////////////////////////////////////////////////////////////////
char PcdRequest(u8 req_code,u8 *pTagType)
{
        char status;
        u8 unLen;
        u8 ucComMF522Buf[MAXRLEN];

        ClearBitMask(Status2Reg,0x08);
        WriteRawRC(BitFramingReg,0x07);
        SetBitMask(TxControlReg,0x03);

        ucComMF522Buf[0] = req_code;

        status = PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,1,ucComMF522Buf,&unLen);

        if ((status == MI_OK) && (unLen == 0x10))
        {
                *pTagType     = ucComMF522Buf[0];
                *(pTagType+1) = ucComMF522Buf[1];
        }
        else
        {   status = MI_ERR;   }

        return status;
}

/////////////////////////////////////////////////////////////////////
//��    �ܣ�����ײ
//����˵��: pSnr[OUT]:��Ƭ���кţ�4�ֽ�
//��    ��: �ɹ�����MI_OK
/////////////////////////////////////////////////////////////////////
char PcdAnticoll(u8 *pSnr)
{
        char status;
        u8 i,snr_check=0;
        u8 unLen;
        u8 ucComMF522Buf[MAXRLEN];


        ClearBitMask(Status2Reg,0x08);
        WriteRawRC(BitFramingReg,0x00);
        ClearBitMask(CollReg,0x80);

        ucComMF522Buf[0] = PICC_ANTICOLL1;
        ucComMF522Buf[1] = 0x20;

        status = PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,2,ucComMF522Buf,&unLen);

        if (status == MI_OK)
        {
                for (i=0; i<4; i++)
                {
                        *(pSnr+i)  = ucComMF522Buf[i];
                        snr_check ^= ucComMF522Buf[i];
                }
                if (snr_check != ucComMF522Buf[i])
                {   status = MI_ERR;    }
        }

        SetBitMask(CollReg,0x80);
        return status;
}

/////////////////////////////////////////////////////////////////////
//��    �ܣ�ѡ����Ƭ
//����˵��: pSnr[IN]:��Ƭ���кţ�4�ֽ�
//��    ��: �ɹ�����MI_OK
/////////////////////////////////////////////////////////////////////
char PcdSelect(u8 *pSnr)
{
        char status;
        u8 i;
        u8 unLen;
        u8 ucComMF522Buf[MAXRLEN];

        ucComMF522Buf[0] = PICC_ANTICOLL1;
        ucComMF522Buf[1] = 0x70;
        ucComMF522Buf[6] = 0;
        for (i=0; i<4; i++)
        {
                ucComMF522Buf[i+2] = *(pSnr+i);
                ucComMF522Buf[6]  ^= *(pSnr+i);
        }
        CalulateCRC(ucComMF522Buf,7,&ucComMF522Buf[7]);

        ClearBitMask(Status2Reg,0x08);

        status = PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,9,ucComMF522Buf,&unLen);

        if ((status == MI_OK) && (unLen == 0x18))
        {   status = MI_OK;  }
        else
        {   status = MI_ERR;    }

        return status;
}

/////////////////////////////////////////////////////////////////////
//��    �ܣ���֤��Ƭ����
//����˵��: auth_mode[IN]: ������֤ģʽ
//                 0x60 = ��֤A��Կ
//                 0x61 = ��֤B��Կ
//          addr[IN]������ַ
//          pKey[IN]������
//          pSnr[IN]����Ƭ���кţ�4�ֽ�
//��    ��: �ɹ�����MI_OK
/////////////////////////////////////////////////////////////////////
char PcdAuthState(u8 auth_mode,u8 addr,u8 *pKey,u8 *pSnr)
{
        char status;
        u8 unLen;
        u8 i,ucComMF522Buf[MAXRLEN];

        ucComMF522Buf[0] = auth_mode;
        ucComMF522Buf[1] = addr;
//    for (i=0; i<6; i++)
//    {    ucComMF522Buf[i+2] = *(pKey+i);   }
//    for (i=0; i<6; i++)
//    {    ucComMF522Buf[i+8] = *(pSnr+i);   }
        memcpy(&ucComMF522Buf[2], pKey, 6);
        memcpy(&ucComMF522Buf[8], pSnr, 4);

        status = PcdComMF522(PCD_AUTHENT,ucComMF522Buf,12,ucComMF522Buf,&unLen);
        if ((status != MI_OK) || (!(ReadRawRC(Status2Reg) & 0x08)))
        {   status = MI_ERR;   }

        return status;
}

/////////////////////////////////////////////////////////////////////
//��    �ܣ���ȡM1��һ������
//����˵��: addr[IN]������ַ
//          p [OUT]�����������ݣ�16�ֽ�
//��    ��: �ɹ�����MI_OK
/////////////////////////////////////////////////////////////////////
char PcdRead(u8 addr,u8 *p )
{
        char status;
        u8 unLen;
        u8 i,ucComMF522Buf[MAXRLEN];

        ucComMF522Buf[0] = PICC_READ;
        ucComMF522Buf[1] = addr;
        CalulateCRC(ucComMF522Buf,2,&ucComMF522Buf[2]);

        status = PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,4,ucComMF522Buf,&unLen);
        if ((status == MI_OK) && (unLen == 0x90))
        //   {   memcpy(p , ucComMF522Buf, 16);   }
        {
                for (i=0; i<16; i++)
                {    *(p +i) = ucComMF522Buf[i];   }
        }
        else
        {   status = MI_ERR;   }

        return status;
}

/////////////////////////////////////////////////////////////////////
//��    �ܣ�д���ݵ�M1��һ��
//����˵��: addr[IN]������ַ
//          p [IN]��д�������ݣ�16�ֽ�
//��    ��: �ɹ�����MI_OK
/////////////////////////////////////////////////////////////////////
char PcdWrite(u8 addr,u8 *p )
{
        char status;
        u8 unLen;
        u8 i,ucComMF522Buf[MAXRLEN];

        ucComMF522Buf[0] = PICC_WRITE;
        ucComMF522Buf[1] = addr;
        CalulateCRC(ucComMF522Buf,2,&ucComMF522Buf[2]);

        status = PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,4,ucComMF522Buf,&unLen);

        if ((status != MI_OK) || (unLen != 4) || ((ucComMF522Buf[0] & 0x0F) != 0x0A))
        {   status = MI_ERR;   }

        if (status == MI_OK)
        {
                //memcpy(ucComMF522Buf, p , 16);
                for (i=0; i<16; i++)
                {
                        ucComMF522Buf[i] = *(p +i);
                }
                CalulateCRC(ucComMF522Buf,16,&ucComMF522Buf[16]);

                status = PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,18,ucComMF522Buf,&unLen);
                if ((status != MI_OK) || (unLen != 4) || ((ucComMF522Buf[0] & 0x0F) != 0x0A))
                {   status = MI_ERR;   }
        }

        return status;
}

/////////////////////////////////////////////////////////////////////
//��    �ܣ����Ƭ��������״̬
//��    ��: �ɹ�����MI_OK
/////////////////////////////////////////////////////////////////////
char PcdHalt(void)
{
        u8 status;
        u8 unLen;
        u8 ucComMF522Buf[MAXRLEN];

        ucComMF522Buf[0] = PICC_HALT;
        ucComMF522Buf[1] = 0;
        CalulateCRC(ucComMF522Buf,2,&ucComMF522Buf[2]);

        status = PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,4,ucComMF522Buf,&unLen);

        return MI_OK;
}

/////////////////////////////////////////////////////////////////////
//��MF522����CRC16����
/////////////////////////////////////////////////////////////////////
void CalulateCRC(u8 *pIn,u8 len,u8 *pOut )
{
        u8 i,n;
        ClearBitMask(DivIrqReg,0x04);
        WriteRawRC(CommandReg,PCD_IDLE);
        SetBitMask(FIFOLevelReg,0x80);
        for (i=0; i<len; i++)
        {   WriteRawRC(FIFODataReg, *(pIn +i));   }
        WriteRawRC(CommandReg, PCD_CALCCRC);
        i = 0xFF;
        do
        {
                n = ReadRawRC(DivIrqReg);
                i--;
        }
        while ((i!=0) && !(n&0x04));
        pOut [0] = ReadRawRC(CRCResultRegL);
        pOut [1] = ReadRawRC(CRCResultRegM);
}

/////////////////////////////////////////////////////////////////////
//��    �ܣ���λRC522
//��    ��: �ɹ�����MI_OK
/////////////////////////////////////////////////////////////////////
char PcdReset(void)
{
        //PORTD|=(1<<RC522RST);
        SET_RC522RST;
        delay_ns(10);
        //PORTD&=~(1<<RC522RST);
        CLR_RC522RST;
        delay_ns(10);
        //PORTD|=(1<<RC522RST);
        SET_RC522RST;
        delay_ns(10);
        WriteRawRC(CommandReg,PCD_RESETPHASE);
        WriteRawRC(CommandReg,PCD_RESETPHASE);
        delay_ns(10);

        WriteRawRC(ModeReg,0x3D);        //��Mifare��ͨѶ��CRC��ʼֵ0x6363
        WriteRawRC(TReloadRegL,30);
        WriteRawRC(TReloadRegH,0);
        WriteRawRC(TModeReg,0x8D);
        WriteRawRC(TPrescalerReg,0x3E);

        WriteRawRC(TxAutoReg,0x40);//����Ҫ

        return MI_OK;
}
//////////////////////////////////////////////////////////////////////
//����RC632�Ĺ�����ʽ
//////////////////////////////////////////////////////////////////////
char M500PcdConfigISOType(u8 type)
{
        if (type == 'A')                //ISO14443_A
        {
                ClearBitMask(Status2Reg,0x08);
                WriteRawRC(ModeReg,0x3D);//3F
                WriteRawRC(RxSelReg,0x86);//84
                WriteRawRC(RFCfgReg,0x7F); //4F
                WriteRawRC(TReloadRegL,30);//tmoLength);// TReloadVal = 'h6a =tmoLength(dec)
                WriteRawRC(TReloadRegH,0);
                WriteRawRC(TModeReg,0x8D);
                WriteRawRC(TPrescalerReg,0x3E);
                delay_ns(1000);
                PcdAntennaOn();
        }
        else{ return 1; }

        return MI_OK;
}
/////////////////////////////////////////////////////////////////////
//��    �ܣ���RC632�Ĵ���
//����˵����Address[IN]:�Ĵ�����ַ
//��    �أ�������ֵ
/////////////////////////////////////////////////////////////////////
u8 ReadRawRC(u8 Address)
{
        u8 ucAddr;
        u8 ucResult=0;
        CLR_SPI_CS;
        ucAddr = ((Address<<1)&0x7E)|0x80;

        SPIWriteByte(ucAddr);
        ucResult=SPIReadByte();
        SET_SPI_CS;
        return ucResult;
}

/////////////////////////////////////////////////////////////////////
//��    �ܣ�дRC632�Ĵ���
//����˵����Address[IN]:�Ĵ�����ַ
//          value[IN]:д����ֵ
/////////////////////////////////////////////////////////////////////
void WriteRawRC(u8 Address, u8 value)
{
        u8 ucAddr;
//	u8 tmp;

        CLR_SPI_CS;
        ucAddr = ((Address<<1)&0x7E);

        SPIWriteByte(ucAddr);
        SPIWriteByte(value);
        SET_SPI_CS;

//	tmp=ReadRawRC(Address);
//
//	if(value!=tmp)
//		printf("wrong\n");
}
/////////////////////////////////////////////////////////////////////
//��    �ܣ���RC522�Ĵ���λ
//����˵����reg[IN]:�Ĵ�����ַ
//          mask[IN]:��λֵ
/////////////////////////////////////////////////////////////////////
void SetBitMask(u8 reg,u8 mask)
{
        char tmp = 0x0;
        tmp = ReadRawRC(reg);
        WriteRawRC(reg,tmp | mask); // set bit mask
}

/////////////////////////////////////////////////////////////////////
//��    �ܣ���RC522�Ĵ���λ
//����˵����reg[IN]:�Ĵ�����ַ
//          mask[IN]:��λֵ
/////////////////////////////////////////////////////////////////////
void ClearBitMask(u8 reg,u8 mask)
{
        char tmp = 0x0;
        tmp = ReadRawRC(reg);
        WriteRawRC(reg, tmp & ~mask); // clear bit mask
}

/////////////////////////////////////////////////////////////////////
//��    �ܣ�ͨ��RC522��ISO14443��ͨѶ
//����˵����Command[IN]:RC522������
//          pIn [IN]:ͨ��RC522���͵���Ƭ������
//          InLenByte[IN]:�������ݵ��ֽڳ���
//          pOut [OUT]:���յ��Ŀ�Ƭ��������
//          *pOutLenBit[OUT]:�������ݵ�λ����
/////////////////////////////////////////////////////////////////////
char PcdComMF522(u8 Command,
                 u8 *pIn,
                 u8 InLenByte,
                 u8 *pOut,
                 u8 *pOutLenBit)
{
        char status = MI_ERR;
        u8 irqEn   = 0x00;
        u8 waitFor = 0x00;
        u8 lastBits;
        u8 n;
        u16 i;
        switch (Command)
        {
        case PCD_AUTHENT:
                irqEn   = 0x12;
                waitFor = 0x10;
                break;
        case PCD_TRANSCEIVE:
                irqEn   = 0x77;
                waitFor = 0x30;
                break;
        default:
                break;
        }

        WriteRawRC(ComIEnReg,irqEn|0x80);
        ClearBitMask(ComIrqReg,0x80); //�������ж�λ
        WriteRawRC(CommandReg,PCD_IDLE);
        SetBitMask(FIFOLevelReg,0x80); //��FIFO����

        for (i=0; i<InLenByte; i++)
        {   WriteRawRC(FIFODataReg, pIn [i]);    }
        WriteRawRC(CommandReg, Command);
//     n = ReadRawRC(CommandReg);

        if (Command == PCD_TRANSCEIVE)
        {    SetBitMask(BitFramingReg,0x80);  }//��ʼ����

        //i = 600;//����ʱ��Ƶ�ʵ���������M1�������ȴ�ʱ��25ms
        i = 2000;
        do
        {
                n = ReadRawRC(ComIrqReg);
                i--;
        }
        while ((i!=0) && !(n&0x01) && !(n&waitFor));
        ClearBitMask(BitFramingReg,0x80);

        if (i!=0)
        {
                if(!(ReadRawRC(ErrorReg)&0x1B))
                {
                        status = MI_OK;
                        if (n & irqEn & 0x01)
                        {   status = MI_NOTAGERR;   }
                        if (Command == PCD_TRANSCEIVE)
                        {
                                n = ReadRawRC(FIFOLevelReg);
                                lastBits = ReadRawRC(ControlReg) & 0x07;
                                if (lastBits)
                                {   *pOutLenBit = (n-1)*8 + lastBits;   }
                                else
                                {   *pOutLenBit = n*8;   }
                                if (n == 0)
                                {   n = 1;    }
                                if (n > MAXRLEN)
                                {   n = MAXRLEN;   }
                                for (i=0; i<n; i++)
                                {   pOut [i] = ReadRawRC(FIFODataReg);    }
                        }
                }
                else
                {   status = MI_ERR;   }

        }


        SetBitMask(ControlReg,0x80);       // stop timer now
        WriteRawRC(CommandReg,PCD_IDLE);
        return status;
}

/////////////////////////////////////////////////////////////////////
//��������
//ÿ���������ر����շ���֮��Ӧ������1ms�ļ���
/////////////////////////////////////////////////////////////////////
void PcdAntennaOn(void)
{
        u8 i;
        i = ReadRawRC(TxControlReg);
        if (!(i & 0x03))
        {
                SetBitMask(TxControlReg, 0x03);
        }
}


/////////////////////////////////////////////////////////////////////
//�ر�����
/////////////////////////////////////////////////////////////////////
void PcdAntennaOff(void)
{
        ClearBitMask(TxControlReg, 0x03);
}

/////////////////////////////////////////////////////////////////////
//��    �ܣ��ۿ��ͳ�ֵ
//����˵��: dd_mode[IN]��������
//               0xC0 = �ۿ�
//               0xC1 = ��ֵ
//          addr[IN]��Ǯ����ַ
//          pValue[IN]��4�ֽ���(��)ֵ����λ��ǰ
//��    ��: �ɹ�����MI_OK
/////////////////////////////////////////////////////////////////////
/*char PcdValue(u8 dd_mode,u8 addr,u8 *pValue)
   {
    char status;
    u8  unLen;
    u8 ucComMF522Buf[MAXRLEN];
    //u8 i;

    ucComMF522Buf[0] = dd_mode;
    ucComMF522Buf[1] = addr;
    CalulateCRC(ucComMF522Buf,2,&ucComMF522Buf[2]);

    status = PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,4,ucComMF522Buf,&unLen);

    if ((status != MI_OK) || (unLen != 4) || ((ucComMF522Buf[0] & 0x0F) != 0x0A))
    {   status = MI_ERR;   }

    if (status == MI_OK)
    {
        memcpy(ucComMF522Buf, pValue, 4);
        //for (i=0; i<16; i++)
        //{    ucComMF522Buf[i] = *(pValue+i);   }
        CalulateCRC(ucComMF522Buf,4,&ucComMF522Buf[4]);
        unLen = 0;
        status = PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,6,ucComMF522Buf,&unLen);
    if (status != MI_ERR)
        {    status = MI_OK;    }
    }

    if (status == MI_OK)
    {
        ucComMF522Buf[0] = PICC_TRANSFER;
        ucComMF522Buf[1] = addr;
        CalulateCRC(ucComMF522Buf,2,&ucComMF522Buf[2]);

        status = PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,4,ucComMF522Buf,&unLen);

        if ((status != MI_OK) || (unLen != 4) || ((ucComMF522Buf[0] & 0x0F) != 0x0A))
        {   status = MI_ERR;   }
    }
    return status;
   }*/

/////////////////////////////////////////////////////////////////////
//��    �ܣ�����Ǯ��
//����˵��: sourceaddr[IN]��Դ��ַ
//          goaladdr[IN]��Ŀ����ַ
//��    ��: �ɹ�����MI_OK
/////////////////////////////////////////////////////////////////////
/*char PcdBakValue(u8 sourceaddr, u8 goaladdr)
   {
    char status;
    u8  unLen;
    u8 ucComMF522Buf[MAXRLEN];

    ucComMF522Buf[0] = PICC_RESTORE;
    ucComMF522Buf[1] = sourceaddr;
    CalulateCRC(ucComMF522Buf,2,&ucComMF522Buf[2]);

    status = PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,4,ucComMF522Buf,&unLen);

    if ((status != MI_OK) || (unLen != 4) || ((ucComMF522Buf[0] & 0x0F) != 0x0A))
    {   status = MI_ERR;   }

    if (status == MI_OK)
    {
        ucComMF522Buf[0] = 0;
        ucComMF522Buf[1] = 0;
        ucComMF522Buf[2] = 0;
        ucComMF522Buf[3] = 0;
        CalulateCRC(ucComMF522Buf,4,&ucComMF522Buf[4]);

        status = PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,6,ucComMF522Buf,&unLen);
    if (status != MI_ERR)
        {    status = MI_OK;    }
    }

    if (status != MI_OK)
    {    return MI_ERR;   }

    ucComMF522Buf[0] = PICC_TRANSFER;
    ucComMF522Buf[1] = goaladdr;

    CalulateCRC(ucComMF522Buf,2,&ucComMF522Buf[2]);

    status = PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,4,ucComMF522Buf,&unLen);

    if ((status != MI_OK) || (unLen != 4) || ((ucComMF522Buf[0] & 0x0F) != 0x0A))
    {   status = MI_ERR;   }

    return status;
   }*/
