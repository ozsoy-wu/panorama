/******************************************************************************
 * Copyright (c) 2015-2018 TP-Link Technologies CO.,LTD.
 *
 * 文件名称:		panorama_utils.c
 * 版           本:	1.0
 * 摘           要:	通用功能
 * 作           者:	wupimin<wupimin@tp-link.com.cn>
 * 创建时间:		2018-04-28
 ******************************************************************************/

#include "panorama_log.h"
#include "panorama_utils.h"
#include "panorama.h"

int cvRound( double value )
{
    return (int)(value + (value >= 0 ? 0.5 : -0.5));
}

