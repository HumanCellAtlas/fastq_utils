/*
 * =========================================================
 * Copyright 2017,  Nuno A. Fonseca (nuno dot fonseca at gmail dot com)
 *
 *
 * This is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License.
 * If not, see <http://www.gnu.org/licenses/>.
 *
 * =========================================================
 */
#include <stdio.h>  
#include <math.h>
#include <bam.h>
#include <sam.h>
#include <kstring.h>      

#include <string.h>
#include <assert.h>

#include "fastq.h"


short get_barcodes(const char *s,char *sample,char *umi,char *cell,
		   int* sample_len, int* umi_len, int* cell_len) {
  cell[0]=sample[0]=umi[0]='\0';

  *cell_len=*sample_len=*umi_len=0;

  if ( s[0]!='_' || s[1]!='S'  || s[2]!='T' || s[3]!='A' ||
       s[4]!='G' || s[5]!='S' || s[6]!='_' ) {
    return(0);
  }
  //
  int idx=7;
  int z=0;
  if ( s[idx]!='C' || s[idx+1]!='E'  || s[idx+2]!='L' || s[idx+3]!='L' ||
       s[idx+4]!='_' ) {
    return(0);
  }
  // cell
  idx=idx+5;
  while (s[idx]!='_') {
    cell[z++]=s[idx++];
  }
  cell[z]='\0';
  *cell_len=z+1;
  idx++;
  // umi
  if ( s[idx]!='U' || s[idx+1]!='M'  || s[idx+2]!='I' || s[idx+3]!='_') {
    return(0);
  }
  idx=idx+4;
  z=0;
  while (s[idx]!='_') {
    umi[z++]=s[idx++];
  }
  umi[z]='\0';
  ++idx;
  *umi_len=z+1;
  // sample
  if ( s[idx]!='S' || s[idx+1]!='A'  || s[idx+2]!='M' || s[idx+3]!='P' ||
       s[idx+4]!='L' || s[idx+5]!='E' || s[idx+6]!='_' ) {
    return(0);
  }
  idx=idx+7;
  z=0;
  while (s[idx]!='_') {
    sample[z++]=s[idx++];
  }
  sample[z]='\0';
  *sample_len=z+1;
  return(1);
}

int main(int argc, char *argv[])  
{  
  short out2stdout=0;
  bamFile in; 
  bamFile out; 

  if (argc != 3) {  
    PRINT_ERROR("Usage: bam_add_tags <in.bam> <out.bam or - for stdout>");  
    return(PARAMS_ERROR_EXIT_STATUS);  
  }  
  // Open file and exit if error
  in = bam_open(argv[1], "rb");
  out2stdout = strcmp(argv[2], "-")? 0 : 1; 
  out = strcmp(argv[2], "-")? bam_open(argv[2], "w") : bam_dopen(fileno(stdout), "w"); 
  if (in == 0 ) {  
    PRINT_ERROR("Failed to open BAM file %s", argv[1]);  
    return(PARAMS_ERROR_EXIT_STATUS);  
  }  

  if (out == 0) {  
    PRINT_ERROR("Failed to open BAM file %s", argv[2]);  
    return(PARAMS_ERROR_EXIT_STATUS);  
  }  

  unsigned long num_alns=0;
  
  // ***********
  // Copy header
  bam_header_t *header;
  header = bam_header_read(in);
  bam_header_write(out,header);

  // 
  bam1_t *aln=bam_init1();

  if (!out2stdout) {
    fprintf(stderr,"bam_add_tags version %s\n",VERSION);
    fprintf(stderr,"Processing %s\n",argv[1]);
  }
  //
  //
  // place holders to keep the info
  char sample[MAX_BARCODE_LENGTH];
  char umi[MAX_BARCODE_LENGTH];
  char cell[MAX_BARCODE_LENGTH];
  int sample_len, umi_len,cell_len;
  num_alns=0;
  while(bam_read1(in,aln)>=0) { // read alignment
    if (aln->core.tid < 0) goto end_loop;//ignore unaligned reads
    if (aln->core.flag & BAM_FUNMAP) goto end_loop;
    ++num_alns;

    //assert(r!=NULL);
    char *qn=bam1_qname(aln);
    //fprintf(stderr,"-->%s\n",qn);
    if (get_barcodes(qn,&sample[0],&umi[0],&cell[0],&sample_len,&umi_len,&cell_len)) {
      //fprintf(stderr,"YES-->%s\n",qn);
      //fprintf(stderr,"YES-->%s %s %s\n",cell,sample,umi);
      if ( umi_len > 0 )
	bam_aux_append(aln, "UM", 'Z', umi_len, umi);
      if ( cell_len > 0 )
	bam_aux_append(aln, "CB", 'Z', cell_len, cell); 
      if ( sample_len > 0 ) // sample index
	bam_aux_append(aln, "BC", 'Z', sample_len, sample); 
    }
  }
 end_loop:
  bam_write1(out,aln);
  bam_close(out); 
  bam_destroy1(aln);  
  return(0);
}