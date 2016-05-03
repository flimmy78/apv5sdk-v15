/*====================================================================*
 *   
 *   Copyright (c) 2011, Atheros Communications Inc.
 *   
 *   Permission to use, copy, modify, and/or distribute this software 
 *   for any purpose with or without fee is hereby granted, provided 
 *   that the above copyright notice and this permission notice appear 
 *   in all copies.
 *   
 *   THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL 
 *   WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED 
 *   WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL  
 *   THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR 
 *   CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM 
 *   LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, 
 *   NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN 
 *   CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *   
 *--------------------------------------------------------------------*/

/*====================================================================*
 *
 *   psgen.h - header for psgen utility
 *
 *   This software and documentation is the property of Atheros 
 *   Corporation, Ocala, Florida. It is provided 'as is' without 
 *   expressed or implied warranty of any kind to anyone for any 
 *   reason. Atheros assumes no responsibility or liability for 
 *   errors or omissions in the software or documentation and 
 *   reserves the right to make changes without notification. 
 *   
 *   Atheros customers may modify and distribute the software 
 *   without obligation to Atheros. Since use of this software 
 *   is optional, users shall bear sole responsibility and 
 *   liability for any consequences of it's use. 
 *
 *.  Qualcomm Atheros HomePlug AV Powerline Toolkit
 *:  Published 2009-2011 by Qualcomm Atheros. ALL RIGHTS RESERVED
 *;  For demonstration and evaluation only. Not for production use
 *
 *   Contributor(s):
 *	Nathaniel Houghton <nathaniel.houghton@qualcomm.com>
 *
 *--------------------------------------------------------------------*/

#ifndef PSGEN_HEADER
#define PSGEN_HEADER

#define _GETOPT_H

/*====================================================================*
 *   system header files;
 *--------------------------------------------------------------------*/

#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*====================================================================*
 *   custom header files;
 *--------------------------------------------------------------------*/

#include "../tools/files.h"

/*====================================================================*
 *   
 *--------------------------------------------------------------------*/

struct fsl3_item 

{
	char *name;
	char *value;
	char *unit;
};

struct fsl3_file 

{
	int item_count;
	int trace_start;
	int trace_end;
	struct fsl3_item *item;
};

struct notch 

{
	double sf;
	double ef;
};

struct notch_set 

{
	int count;
	double depth;
	struct notch *notch;
};

struct dev_config 

{
	struct device_spec *dspec;
	uint32_t *prescaler;
	int gain_adj;
	uint32_t truncated;
	double average;
};

struct device_spec 

{
	const char *name;
	unsigned prescaler_count;
	unsigned prescaler_unity;
	unsigned prescaler_max;
	int tail_start;
	uint32_t (*freq_to_index)(double, struct device_spec *);
	double (*index_to_freq)(uint32_t, struct device_spec *);
	uint32_t (*amp_to_ps)(double, struct device_spec *);
	double (*ps_to_amp)(uint32_t, struct device_spec *);
	int (*update_pib)(const char *, struct dev_config *);
	int (*set_tx_gain)(struct _file_ *, int);
	int (*check_tx_gain)(int);
};

struct trace 

{
	int count;
	double *freq;
	double *value;
};

struct prescalers 

{
	int count;
	int gain_adj;
	uint32_t *value;
};

struct tweak 

{
	double sf;
	double ef;
	double sv;
	double ev;
	struct tweak *next;
};


#define TWEAK_ABSOLUTE 0
#define TWEAK_RELATIVE 1

const char *strtodouble (const char *, double *);
double estimate_trace_value (struct trace *, double);
int create_trace_fsl3 (struct fsl3_file *, struct trace *);
int load_fsl3 (const char *, struct fsl3_file *);
void free_fsl3_file (struct fsl3_file *);
void free_tweaks (struct tweak *);
int parse_tweak (struct tweak *, const char *);
int create_trace_copy (struct trace *, struct trace *);
int reshape_trace (struct trace *, struct trace *, struct device_spec *);
void free_trace_data (struct trace *);
int find_trace_index (struct trace *, double);
double estimate_trace_value (struct trace *, double);
int apply_tweak (struct trace *, struct tweak *, int);
int output_prescalers (struct trace *, struct trace *, struct device_spec *, int);

/* currently generic */

double ps_to_amp (uint32_t, struct device_spec *);
uint32_t amp_to_ps (double, struct device_spec *);
uint32_t freq_to_index (double, struct device_spec *);
double index_to_freq (uint32_t, struct device_spec *);

/* lynx/panther */

uint32_t lynx_freq_to_index (double, struct device_spec *);
double lynx_index_to_freq (uint32_t, struct device_spec *);
uint32_t panther_freq_to_index (double, struct device_spec *);
double panther_index_to_freq (uint32_t, struct device_spec *);
int psin (struct _file_ *, struct dev_config *);
int update_pib (const char *, struct dev_config *);
int set_tx_gain_6400 (struct _file_ *, int);
int set_tx_gain_7400 (struct _file_ *, int);
int check_tx_gain_6400 (int);
int check_tx_gain_7400 (int);
void dump_trace (struct trace *);
struct dev_config *generate_config (struct trace *, struct trace *, struct device_spec *, int);
int print_config (struct dev_config *);
void free_dev_config (struct dev_config *);

/*====================================================================*
 *   
 *--------------------------------------------------------------------*/

#ifdef WIN32

double rint (double);

#endif

/*====================================================================*
 *   
 *--------------------------------------------------------------------*/

#endif

