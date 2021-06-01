
#pragma once

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

/* Select a dataset below by uncommenting it.
Then modify the file location and parameters below in the Parameters section. */

// Note that gtruthindc require there to be no new lines, so remove new lines
// in those files (replace with spaces if needed)

// #define URL
// #define WEBSPAM_TRI
// #define DNA_FULL_GENOME
// #define PROMETHION_SHORT
#define YFCC
// #define DNA_FULL_PROTEOME

/* Parameters. */
#if defined URL

#define SPARSEDATASET

#define NUMHASHBATCH				200

#define K					4
#define RANGE   				17

#define DIMENSION				120
#define FULL_DIMENSION				3231961

#define NUMBASE					2386130
#define NUMQUERY				10000
#define AVAILABLE_TOPK				1024
#define TOPK					128

#define BASEFILE		"../Data/Url/data"
#define GTRUTHINDICE	        "../Data/Url/indices"

#elif defined WEBSPAM_TRI

#define SPARSEDATASET

#define NUMHASHBATCH				50

#define K					4
#define RANGE                                   18

#define DIMENSION				4000
#define FULL_DIMENSION				16609143

#define NUMBASE					340000
#define NUMQUERY				10000
#define AVAILABLE_TOPK				1024
#define TOPK					128

#define BASEFILE	        "../Data/Webspam/data"
#define GTRUTHINDICE	        "../Data/Webspam/indices"

#elif defined DNA_FULL_GENOME

#define SETDATASET

#define NUMHASHBATCH				50

#define K					1
#define RANGE                                   17

#define NUMBASE					117219
#define NUMQUERY				10000
#define TOPK					128
#define AVAILABLE_TOPK				128

#define BASEFILE	        "../Data/Genomes/data"
#define GTRUTHINDICE	        "../Data/Genomes/indices"

#elif defined DNA_FULL_PROTEOME

#define SETDATASET

#define NUMHASHBATCH				50

#define K					1
#define RANGE                                   17

#define NUMBASE					116373
#define NUMQUERY				10000
#define TOPK					128
#define AVAILABLE_TOPK				128

#define BASEFILE	        "../Data/Proteomes/data"
#define GTRUTHINDICE	        "../Data/Proteomes/indices"

#elif defined PROMETHION_SHORT

#define SETDATASET

#define NUMHASHBATCH				200

#define K					1
#define RANGE                                   17

#define NUMBASE					3696341
#define NUMQUERY				10000
#define TOPK					128
#define AVAILABLE_TOPK				128

#define BASEFILE	        "../Data/Promethion/data"
#define GTRUTHINDICE	        "../Data/Promethion/indices"
#define GTRUTHDIST		"../Data/Promethion/distances"

#elif defined YFCC 

#define DENSEDATASET

// #define NUMBASE                                 96970001
#define NUMBASE                                 1200000
#define NUMQUERY				50
#define TOPK					100
#define AVAILABLE_TOPK				100
#define NUMHASHBATCH				20

#define DIMENSION				4096
#define RANGE   				14

#define BASEFILE                "/home/jae4/Data/Yfcc/data"
#define GTRUTHINDICE            "/home/jae4/Data/Yfcc/indices"
#define QUERYFILE               "/home/jae4/Data/Yfcc/queries"


#endif

#define STARTREPS                               (1<<2)
#define MAXREPS                                 (1<<11)
#define REPRATIO                                2

#define RESRATIO                                2
#define BRATIO                                  2


#ifdef YFCC
        #define STARTB                          (1<<12)
        #define ENDB                            (1<<19)
        #define STARTR                          2
        #define ENDR                            2
        #define FLINNG32BIT

        #define STARTRES                        (1<<3)
        #define ENDRES                          (1<<12)
#else
        #define STARTB                          (1<<11)
        #define ENDB                            (1<<15)
        #define STARTR                          2
        #define ENDR                            4 
        #define FLINNG16BIT

        #define STARTRES                        (1<<2)
        #define ENDRES                          (1<<11)
#endif

// To avoid annoying compile time errors
#ifndef DIMENSION
        #define DIMENSION 0 
#endif
#ifndef QUERYFILE
        #define QUERYFILE ""
#endif

