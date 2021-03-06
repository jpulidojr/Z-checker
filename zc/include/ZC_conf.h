/**
 *  @file ZC_conf.h
 *  @author Sheng Di
 *  @date April, 2015
 *  @brief Header file for the ZC_conf.c.
 *  (C) 2015 by Mathematics and Computer Science (MCS), Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef _ZC_Conf_H
#define _ZC_Conf_H

#include <stdio.h>
#include "zc.h"

#ifdef _WIN32
#define PATH_SEPARATOR ';'
#else
#define PATH_SEPARATOR ':'
#endif

#ifdef __cplusplus
extern "C" {
#endif

void loadProperty(char* property_dir, char* propertyVarName);
int ZC_ReadConf();
int ZC_LoadConf();
int modifyZCConfig(StringLine* confLinesHeader, char* targetAttribute, char* newStringValue);

#ifdef __cplusplus
}
#endif

#endif /* ----- #ifndef _ZC_Conf_H  ----- */
