#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "clustalw.h"	

static void make_p_ptrs(sint *tptr, sint *pl, char *data, sint l);
static void make_n_ptrs(sint *tptr, sint *pl, char *data, sint len);
static void put_frag(sint fs, sint v1, sint v2, sint flen);
static sint frag_rel_pos(sint a1, sint b1, sint a2, sint b2);
static void des_quick_sort(sint *array1, sint *array2, sint array_size);
static void pair_align(Boolean dnaflag,char *data, sint l1, sint l2,sint max_aln_length);

#define MIN(a,b) ((a)<(b)?(a):(b))
#define MAX(a,b) ((a)>(b)?(a):(b))


/*
*	Prototypes
*/

/*
*	 Global variables
*/

static sint 	next;
static sint 	curr_frag,maxsf;
static sint 	**accum;
static sint 	*diag_index;
static char 	*slopes;

static sint ktup,window,wind_gap,signif;    		      /* Pairwise aln. params */
sint *displ;
sint *zza, *zzb, *zzc, *zzd;



static void make_p_ptrs(sint *tptr,sint *pl,char *data,sint l)
{
	static sint a[10];
	sint i,j,limit,code,flag;
	char residue;
	
	for (i=1;i<=ktup;i++)
           a[i] = (sint) pow((double)(NUMRES+1),(double)(i-1));

	limit = (sint) pow((double)(NUMRES+1),(double)ktup);
	for(i=1;i<=limit;i++)
		pl[i]=0;
	for(i=1;i<=l;i++)
		tptr[i]=0;
	
	for(i=1;i<=(l-ktup+1);i++) {
		code=0;
		flag=FALSE;
		for(j=1;j<=ktup;++j) {
			residue = data[i+j-2];
			if(!isalpha(residue)){
				flag=TRUE;
				break;
			}
			code += ((residue-'a') * a[j]);
		}
		if(flag)
			continue;
		++code;
		if(pl[code]!=0)
			tptr[i]=pl[code];
		pl[code]=i;
	}
}


static void make_n_ptrs(sint *tptr,sint *pl,char *data,sint len)
{
	static sint pot[]={ 0, 1, 4, 16, 64, 256, 1024, 4096 };
	sint i,j,limit,code,flag;
	char residue;
	
	limit = (sint) pow((double)4,(double)ktup);
	
	for(i=0;i<limit;i++)
		pl[i]=0;
	for(i=0;i<len;i++)
		tptr[i]=0;
	
	for(i=0;i<len-ktup+1;i++) {
		code=0;
		flag=FALSE;
		for(j=0;j<ktup;j++) {
			residue = data[i+j];
			if((residue<0) || (residue>4)){
				flag=TRUE;
				break;
			}
			code += ((residue-'a') * pot[j]);  /* DES */
		}
		if(flag)
			continue;
		++code;
		if(pl[code]!=0)
			tptr[i]=pl[code];
		pl[code]=i;
	}
}


static void put_frag(sint fs,sint v1,sint v2,sint flen)
{
	sint end;
	accum[0][curr_frag]=fs;
	accum[1][curr_frag]=v1;
	accum[2][curr_frag]=v2;
	accum[3][curr_frag]=flen;
	
	if(!maxsf) {
		maxsf=1;
		accum[4][curr_frag]=0;
		return;
	}
	
        if(fs >= accum[0][maxsf]) {
		accum[4][curr_frag]=maxsf;
		maxsf=curr_frag;
		return;
	}
	else {
		next=maxsf;
		while(TRUE) {
			end=next;
			next=accum[4][next];
			if(fs>=accum[0][next])
				break;
		}
		accum[4][curr_frag]=next;
		accum[4][end]=curr_frag;
	}
}


static sint frag_rel_pos(sint a1,sint b1,sint a2,sint b2)
{
	sint ret;
	
	ret=FALSE;
	if(a1-b1==a2-b2) {
		if(a2<a1)
			ret=TRUE;
	}
	else {
		if(a2+ktup-1<a1 && b2+ktup-1<b1)
			ret=TRUE;
	}
	return ret;
}


static void des_quick_sort(sint *array1, sint *array2, sint array_size)
/*  */
/* Quicksort routine, adapted from chapter 4, page 115 of software tools */
/* by Kernighan and Plauger, (1986) */
/* Sort the elements of array1 and sort the */
/* elements of array2 accordingly */
/*  */
{
	sint temp1, temp2;
	sint p, pivlin;
	sint i, j;
	sint lst[50], ust[50];       /* the maximum no. of elements must be*/
								/* < log(base2) of 50 */

	lst[1] = 0;
	ust[1] = array_size-1;
	p = 1;

	while(p > 0) {
		if(lst[p] >= ust[p])
			p--;
		else {
			i = lst[p] - 1;
			j = ust[p];
			pivlin = array1[j];
			while(i < j) {
				for(i=i+1; array1[i] < pivlin; i++)
					;
				for(j=j-1; j > i; j--)
					if(array1[j] <= pivlin) break;
				if(i < j) {
					temp1     = array1[i];
					array1[i] = array1[j];
					array1[j] = temp1;
					
					temp2     = array2[i];
					array2[i] = array2[j];
					array2[j] = temp2;
				}
			}
			
			j = ust[p];

			temp1     = array1[i];
			array1[i] = array1[j];
			array1[j] = temp1;

			temp2     = array2[i];
			array2[i] = array2[j];
			array2[j] = temp2;

			if(i-lst[p] < ust[p] - i) {
				lst[p+1] = lst[p];
				ust[p+1] = i - 1;
				lst[p]   = i + 1;
			}
			else {
				lst[p+1] = i + 1;
				ust[p+1] = ust[p];
				ust[p]   = i - 1;
			}
			p = p + 1;
		}
	}
	return;

}





static void pair_align(Boolean dnaflag,char *data,sint l1,sint l2,sint max_aln_length)
{
	sint pot[8],i,j,l,m,flag,limit,pos,tl1,vn1,vn2,flen,osptr,fs;
	sint tv1,tv2,encrypt,subt1,subt2,rmndr;
	char residue;
	
	if(dnaflag) {
		for(i=1;i<=ktup;i++)
			pot[i] = (sint) pow((double)4,(double)(i-1));
		limit = (sint) pow((double)4,(double)ktup);
	}
	else {
		for (i=1;i<=ktup;i++)
           		pot[i] = (sint) pow((double)(NUMRES+1),(double)(i-1));
		limit = (sint) pow((double)(NUMRES+1),(double)ktup);
	}
	
	tl1 = (l1+l2)-1;
	
	for(i=1;i<=tl1;i++) {
		slopes[i]=displ[i]=0;
		diag_index[i] = i;
	}
	

/* increment diagonal score for each k_tuple match */

	for(i=1;i<=limit;i++) {
		vn1=zzc[i];
		while(TRUE) {
			if(!vn1) break;
			vn2=zzd[i];
			while(vn2 != 0) {
				osptr=vn1-vn2+l2;
				++displ[osptr];
				vn2=zzb[vn2];
			}
			vn1=zza[vn1];
		}
	}

/* choose the top SIGNIF diagonals */

	des_quick_sort(displ, diag_index, tl1);

	j = tl1 - signif + 1;
	if(j < 1) j = 1;
 
/* flag all diagonals within WINDOW of a top diagonal */

	for(i=tl1; i>=j; i--) 
		if(displ[i] > 0) {
			pos = diag_index[i];
			l = (1  >pos-window) ? 1   : pos-window;
			m = (tl1<pos+window) ? tl1 : pos+window;
			for(; l <= m; l++) 
				slopes[l] = 1;
		}

	for(i=1; i<=tl1; i++)  displ[i] = 0;

	
	curr_frag=maxsf=0;
	
	for(i=1;i<=(l1-ktup+1);i++) {
		encrypt=flag=0;
		for(j=1;j<=ktup;j++) {
			residue = data[i+j-2];
			if(!isalpha(residue)) {
				flag=TRUE;
				break;
			}
			encrypt += ((residue-'a')*pot[j]);
		}
		if(flag) continue;
		++encrypt;
	
		vn2=zzd[encrypt];
	
		flag=FALSE;
		while(TRUE) {
			if(!vn2) {
				flag=TRUE;
				break;
			}
			osptr=i-vn2+l2;
			if(slopes[osptr]!=1) {
				vn2=zzb[vn2];
				continue;
			}
			flen=0;
			fs=ktup;
			next=maxsf;		
		
		/*
		* A-loop
		*/
		
			while(TRUE) {
				if(!next) {
					++curr_frag;
					if(curr_frag>=2*max_aln_length) {
						info("(Partial alignment)");
						return;
					}
					displ[osptr]=curr_frag;
					put_frag(fs,i,vn2,flen);
				}
				else {
					tv1=accum[1][next];
					tv2=accum[2][next];
					if(frag_rel_pos(i,vn2,tv1,tv2)) {
						if(i-vn2==accum[1][next]-accum[2][next]) {
							if(i>accum[1][next]+(ktup-1))
								fs=accum[0][next]+ktup;
							else {
								rmndr=i-accum[1][next];
								fs=accum[0][next]+rmndr;
							}
							flen=next;
							next=0;
							continue;
						}
						else {
							if(displ[osptr]==0)
								subt1=ktup;
							else {
								if(i>accum[1][displ[osptr]]+(ktup-1))
									subt1=accum[0][displ[osptr]]+ktup;
								else {
									rmndr=i-accum[1][displ[osptr]];
									subt1=accum[0][displ[osptr]]+rmndr;
								}
							}
							subt2=accum[0][next]-wind_gap+ktup;
							if(subt2>subt1) {
								flen=next;
								fs=subt2;
							}
							else {
								flen=displ[osptr];
								fs=subt1;
							}
							next=0;
							continue;
						}
					}
					else {
						next=accum[4][next];
						continue;
					}
				}
				break;
			}
		/*
		* End of Aloop
		*/
		
			vn2=zzb[vn2];
		}
	}
}		 

void show_pair(ALN mult_aln,sint istart, sint iend, sint jstart, sint jend,double **tmat,QUICKPW_OPT quickpw_opt,Boolean verbose)
{
	sint i,j,k,dsr;
	double calc_score;
	sint max_aln_length;
	
	max_aln_length=0;
        for(i=istart;i<iend;i++)
               	if(mult_aln.seqs[i].len>max_aln_length)
                        max_aln_length=mult_aln.seqs[i].len;
       	for(j=jstart;j<jend;j++)
               	if(mult_aln.seqs[j].len>max_aln_length)
                        max_aln_length=mult_aln.seqs[j].len;
	max_aln_length*=2;

	accum = (sint **)ckalloc( 5*sizeof (sint *) );
	for (i=0;i<5;i++)
		accum[i] = (sint *) ckalloc((2*max_aln_length+1) * sizeof (sint) );

	displ      = (sint *) ckalloc( (2*max_aln_length +1) * sizeof (sint) );
	slopes     = (char *)ckalloc( (2*max_aln_length +1) * sizeof (char));
	diag_index = (sint *) ckalloc( (2*max_aln_length +1) * sizeof (sint) );

	zza = (sint *)ckalloc( (max_aln_length+1) * sizeof (sint) );
	zzb = (sint *)ckalloc( (max_aln_length+1) * sizeof (sint) );

	zzc = (sint *)ckalloc( (max_aln_length+1) * sizeof (sint) );
	zzd = (sint *)ckalloc( (max_aln_length+1) * sizeof (sint) );

        if(mult_aln.dnaflag) {
                ktup     = quickpw_opt.dna_ktup;
                window   = quickpw_opt.dna_window;
                signif   = quickpw_opt.dna_signif;
                wind_gap = quickpw_opt.dna_wind_gap;
        }
        else {
                ktup     = quickpw_opt.prot_ktup;
                window   = quickpw_opt.prot_window;
                signif   = quickpw_opt.prot_signif;
                wind_gap = quickpw_opt.prot_wind_gap;
        }

	fprintf(stdout,"\n\n");
	
	for(i=istart;i<iend;i++) {
		if(mult_aln.dnaflag)
			make_n_ptrs(zza,zzc,mult_aln.seqs[i].data,mult_aln.seqs[i].len);
		else
			make_p_ptrs(zza,zzc,mult_aln.seqs[i].data,mult_aln.seqs[i].len);
		for(j=MAX(i+1,jstart);j<jend;j++) {
			if(mult_aln.dnaflag)
				make_n_ptrs(zzb,zzd,mult_aln.seqs[j].data,mult_aln.seqs[j].len);
			else
				make_p_ptrs(zzb,zzd,mult_aln.seqs[j].data,mult_aln.seqs[j].len);
			pair_align(mult_aln.dnaflag,mult_aln.seqs[i].data,mult_aln.seqs[i].len,mult_aln.seqs[j].len,max_aln_length);
			if(!maxsf)
				calc_score=0.0;
			else {
				calc_score=(double)accum[0][maxsf];
				if(quickpw_opt.percent) {
					dsr=(mult_aln.seqs[i].len<mult_aln.seqs[j].len) ?
							mult_aln.seqs[i].len : mult_aln.seqs[j].len;
				calc_score = (calc_score/(double)dsr) * 100.0;
				}
			}
/*
			tmat[i][j]=calc_score;
			tmat[j][i]=calc_score;
*/

                        tmat[i][j] = (100.0 - calc_score)/100.0;
                        tmat[j][i] = (100.0 - calc_score)/100.0;
			if(calc_score>0.1) 
				if(verbose) info("Sequences (%d:%d) Aligned. Score: %lg",
               			(pint)i+1,(pint)j+1,calc_score);
			else
				if(verbose) info("Sequences (%d:%d) Not Aligned",
						(pint)i,(pint)j);
		}
	}

	for (i=0;i<5;i++)
	   accum[i]=ckfree((void *)accum[i]);
	accum=ckfree((void *)accum);

	displ=ckfree((void *)displ);
	slopes=ckfree((void *)slopes);
	diag_index=ckfree((void *)diag_index);

	zza=ckfree((void *)zza);
	zzb=ckfree((void *)zzb);
	zzc=ckfree((void *)zzc);
	zzd=ckfree((void *)zzd);
}

