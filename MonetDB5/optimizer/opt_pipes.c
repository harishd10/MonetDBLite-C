/*
 * The contents of this file are subject to the MonetDB Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.monetdb.org/Legal/MonetDBLicense
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See the
 * License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is the MonetDB Database System.
 *
 * The Initial Developer of the Original Code is CWI.
 * Portions created by CWI are Copyright (C) 1997-July 2008 CWI.
 * Copyright August 2008-2011 MonetDB B.V.
 * All Rights Reserved.
 */

/*
 * @f opt_pipes
 * @a M.L. Kersten
 * @-
 * The default SQL optimizer pipeline can be set per server.  See the
 * optpipe setting in monetdb(1) when using merovingian.  During SQL
 * initialization, the optimizer pipeline is checked against the
 * dependency information maintained in the optimizer library to ensure
 * there are no conflicts and at least the pre-requisite optimizers are
 * used.  The setting of sql_optimizer can be either the list of
 * optimizers to run, or one or more variables containing the optimizer
 * pipeline to run.  The latter is provided for readability purposes
 * only.
 */
#include "monetdb_config.h"
#include "opt_pipes.h"

#define MAXOPTPIPES 64

struct PIPELINES{
	char name[50];
	char def[256];
} pipes[MAXOPTPIPES] ={
/* The minimal pipeline necessary by the server to operate correctly*/
{ "minimal_pipe",	"inline,remap,deadcode,multiplex,garbageCollector"},

/*
 * The default pipe line contains as of Feb2010 mitosis-mergetable-reorder,
 * aimed at large tables and improved access locality
*/
{ "default_pipe",	"inline,remap,evaluate,costModel,coercions,emptySet,aliases,mitosis,mergetable,deadcode,commonTerms,joinPath,reorder,deadcode,reduce,dataflow,history,multiplex,garbageCollector" },

/*
 * The no_mitosis pipe line is (and should be kept!) identical to the default pipeline,
 * except that optimizer mitosis is omitted.  It is used mainly to make some tests work
 * deterministically, and to check / debug whether "unexpected" problems are related to
 * mitosis (and/or mergetable).
*/
{ "no_mitosis_pipe",	"inline,remap,evaluate,costModel,coercions,emptySet,aliases,mergetable,deadcode,commonTerms,joinPath,reorder,deadcode,reduce,dataflow,history,multiplex,garbageCollector" },

/* The sequential pipe line is (and should be kept!) identical to the default pipeline,
 * except that optimizers mitosis & dataflow are omitted.  It is use mainly to make some
 * tests work deterministically, i.e., avoid ambigious output, by avoiding parallelism.
*/
{ "sequential_pipe",	"inline,remap,evaluate,costModel,coercions,emptySet,aliases,mergetable,deadcode,commonTerms,joinPath,reorder,deadcode,reduce,history,multiplex,garbageCollector" },

/* The default pipeline used in the November 2009 release*/
{ "nov2009_pipe",	"inline,remap,evaluate,costModel,coercions,emptySet,aliases,mergetable,deadcode,constants,commonTerms,joinPath,deadcode,reduce,dataflow,history,multiplex,garbageCollector" },

/*
 * Experimental pipelines stressing various components under development
 * Do not use any of these pipelines in production settings!
*/
{"replication_pipe",	"inline,remap,evaluate,costModel,coercions,emptySet,aliases,mergetable,deadcode,constants,commonTerms,joinPath,deadcode,reduce,dataflow,history,replication,multiplex,garbageCollector" },

{"accumulator_pipe",	"inline,remap,evaluate,costModel,coercions,emptySet,aliases,mergetable,deadcode,constants,commonTerms,joinPath,deadcode,reduce,accumulators,dataflow,history,multiplex,garbageCollector"},

{"recycler_pipe",	"inline,remap,evaluate,costModel,coercions,emptySet,aliases,deadcode,constants,commonTerms,joinPath,deadcode,recycle,reduce,dataflow,history,multiplex,garbageCollector"},

{"cracker_pipe",	"inline,remap,evaluate,costModel,coercions,emptySet,aliases,selcrack,deadcode,commonTerms,joinPath,reorder,deadcode,reduce,dataflow,history,multiplex,garbageCollector"},
{"sidcrack_pipe",	"inline,remap,evaluate,costModel,coercions,emptySet,aliases,sidcrack,deadcode,commonTerms,joinPath,reorder,deadcode,reduce,dataflow,history,multiplex,garbageCollector"},

/*
 * The Octopus pipeline for distributed processing (Merovingian enabled platforms only)
*/
{"octopus_pipe",	"inline,remap,evaluate,costModel,coercions,emptySet,aliases,mitosis,mergetable,deadcode,commonTerms,joinPath,reorder,deadcode,costModel,octopus,reduce,dataflow,history,multiplex,garbageCollector"},

{"datacell_pipe",
            "inline,remap,datacell,garbageCollector,evaluate,costModel,coercions,emptySet,aliases,mitosis,"
            "mergetable,deadcode,commonTerms,joinPath,reorder,deadcode,reduce,dataflow,"
            "history,multiplex,accumulators,garbageCollector" },
/* The default + datacyclotron*/
{"datacyclotron_pipe",	"inline,remap,evaluate,costModel,coercions,emptySet,aliases,datacyclotron,mergetable,deadcode,commonTerms,joinPath,reorder,deadcode,reduce,dataflow,history,replication,multiplex,garbageCollector"},

/* The default + derivePath" */
{"derive_pipe",	"inline,remap,evaluate,costModel,coercions,emptySet,aliases,mitosis,mergetable,deadcode,commonTerms,derivePath,joinPath,reorder,deadcode,reduce,dataflow,history,multiplex,garbageCollector"},

/* The default + dictionary*/
{"dictionary_pipe",	"inline,remap,dictionary,evaluate,costModel,coercions,emptySet,aliases,mergetable,deadcode,constants,commonTerms,joinPath,deadcode,reduce,dataflow,history,multiplex,garbageCollector"},

/* The default + compression */
{"compression_pipe",	"inline,remap,evaluate,costModel,coercions,emptySet,aliases,mergetable,deadcode,constants,commonTerms,joinPath,deadcode,reduce,dataflow,compression,dataflow,history,multiplex,garbageCollector"},

/* 
 * The centipede pipe line aims at a map-reduce style of query processing
*/
{ "centipede",	"inline,remap,evaluate,costModel,coercions,emptySet,aliases,centipede,mergetable,deadcode,commonTerms,joinPath,reorder,deadcode,reduce,dataflow,history,multiplex,garbageCollector" }

};
/*
 * @-
 * Debugging the optimizer pipeline",
 * The best way is to use mdb and inspect the information gathered",
 * during the optimization phase.  Several optimizers produce more",
 * intermediate information, which may shed light on the details.  The",
 * opt_debug bitvector controls their output. It can be set to a",
 * pipeline or a comma separated list of optimizers you would like to",
 * trace. It is a server wide property and can not be set dynamically,",
 * as it is intended for internal use.",
 */
#include "opt_pipes.h"

str
addPipeDefinition(str name, str pipe)
{
	int i;
	for( i =0; i< MAXOPTPIPES; i++)
	if ( pipes[i].name && strcmp(name,pipes[i].name)==0)
		return NULL;
	else
	if ( pipes[i].name == 0){
		snprintf(pipes[i].name,50,"%s",name);
		snprintf(pipes[i].def,256,"%s",pipe);
		return NULL;
	}
	return NULL;
}

str
getPipeDefinition(str name)
{
	int i;

	for( i=0; i < MAXOPTPIPES && *pipes[i].name; i++)
		if ( strcmp(name, pipes[i].name) == 0)
			return GDKstrdup(pipes[i].def);
	return NULL;
}

str
getPipeCatalog(int *nme, int *def)
{
	BAT *b,*bn;
	int i;
	b= BATnew(TYPE_void, TYPE_str, 20);
	if ( b == NULL)
		throw(MAL, "optimizer.getpipeDefinition", MAL_MALLOC_FAIL );
	bn= BATnew(TYPE_void, TYPE_str, 20);
	if ( bn == NULL){
		BBPreleaseref(b->batCacheid);
		throw(MAL, "optimizer.getpipeDefinition", MAL_MALLOC_FAIL );
	}
	BATseqbase(b,0);
	BATseqbase(bn,0);
	for( i=0; i < MAXOPTPIPES && *pipes[i].name; i++){
		BUNappend(b,pipes[i].name, FALSE);
		BUNappend(bn,pipes[i].def, FALSE);
	}

	BBPkeepref(*nme= b->batCacheid);
	BBPkeepref(*def= b->batCacheid);
	return MAL_SUCCEED;
}
