/*
 * rk_aiq_algo_aeis_itf.h
 *
 *  Copyright (c) 2021 Rockchip Electronics Co., Ltd
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Author: Cody Xie <cody.xie@rock-chips.com>
 */
#ifndef _RK_AIQ_ALGO_AEIS_ITF_H_
#define _RK_AIQ_ALGO_AEIS_ITF_H_

#include "rk_aiq_algo_des.h"

#define RKISP_ALGO_AEIS_VERSION "v0.0.1"
#define RKISP_ALGO_AEIS_VENDOR "Rockchip"
#define RKISP_ALGO_AEIS_DESCRIPTION "Rockchip Aeis algo for ISP2.0"

XCAM_BEGIN_DECLARE

extern RkAiqAlgoDescription g_RkIspAlgoDescAeis;

XCAM_END_DECLARE

#endif //_RK_AIQ_ALGO_AEIS_ITF_H_
