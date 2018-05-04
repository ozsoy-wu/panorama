/******************************************************************************
 * Copyright (c) 2015-2018 TP-Link Technologies CO.,LTD.
 *
 * �ļ�����:		panorama_utils.c
 * ��           ��:	1.0
 * ժ           Ҫ:	ͨ�ù���
 * ��           ��:	wupimin<wupimin@tp-link.com.cn>
 * ����ʱ��:		2018-04-28
 ******************************************************************************/

#include "panorama_log.h"
#include "panorama_utils.h"
#include "panorama.h"

int cvRound( double value )
{
    return (int)(value + (value >= 0 ? 0.5 : -0.5));
}

