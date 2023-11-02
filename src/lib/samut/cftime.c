/*
 *
 * this is the reimplementation of the libc function cftime because the 
 * function ascftime 
 * is bad implemented for the 64bit environment

 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at pkg/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at pkg/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 * Copyright 2023 Carsten Grzemba
*/

#include <stdlib.h>
#include "sam/lib.h"

#define BUFFSIZ MAXPATHLEN

int
samcftime(char *buf, const char *format, const time_t *t)  
{
        struct tm *p;  

        p = localtime(t);
        if (p == NULL) {
                *buf = '\0';
                return (0); 
        }

        if (format == NULL || *format == '\0')
		format =  "%+";

        return ((int)strftime(buf, BUFFSIZ, format, p)); 
}
