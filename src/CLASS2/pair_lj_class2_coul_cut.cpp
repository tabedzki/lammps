/* ----------------------------------------------------------------------
   LAMMPS - Large-scale Atomic/Molecular Massively Parallel Simulator
   http://lammps.sandia.gov, Sandia National Laboratories
   Steve Plimpton, sjplimp@sandia.gov

   Copyright (2003) Sandia Corporation.  Under the terms of Contract
   DE-AC04-94AL85000 with Sandia Corporation, the U.S. Government retains
   certain rights in this software.  This software is distributed under 
   the GNU General Public License.

   See the README file in the top-level LAMMPS directory.
------------------------------------------------------------------------- */

#include "math.h"
#include "stdio.h"
#include "stdlib.h"
#include "pair_lj_class2_coul_cut.h"
#include "atom.h"
#include "comm.h"
#include "force.h"
#include "update.h"
#include "memory.h"
#include "neighbor.h"
#include "error.h"

using namespace LAMMPS_NS;

#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))

/* ---------------------------------------------------------------------- */

PairLJClass2CoulCut::PairLJClass2CoulCut(LAMMPS *lmp) : Pair(lmp) {}

/* ---------------------------------------------------------------------- */

PairLJClass2CoulCut::~PairLJClass2CoulCut()
{
  if (allocated) {
    memory->destroy_2d_int_array(setflag);
    memory->destroy_2d_double_array(cutsq);

    memory->destroy_2d_double_array(cut_lj);
    memory->destroy_2d_double_array(cut_ljsq);
    memory->destroy_2d_double_array(cut_coul);
    memory->destroy_2d_double_array(cut_coulsq);
    memory->destroy_2d_double_array(epsilon);
    memory->destroy_2d_double_array(sigma);
    memory->destroy_2d_double_array(lj1);
    memory->destroy_2d_double_array(lj2);
    memory->destroy_2d_double_array(lj3);
    memory->destroy_2d_double_array(lj4);
    memory->destroy_2d_double_array(offset);
  }
}

/* ---------------------------------------------------------------------- */

void PairLJClass2CoulCut::compute(int eflag, int vflag)
{
  int i,j,k,numneigh,itype,jtype;
  double qtmp,xtmp,ytmp,ztmp,delx,dely,delz;
  double rsq,rinv,r2inv,r3inv,r6inv,forcecoul,forcelj,fforce;
  double factor_coul,factor_lj,factor,phicoul,philj;
  int *neighs;
  double **f;

  eng_vdwl = eng_coul = 0.0;
  if (vflag) for (i = 0; i < 6; i++) virial[i] = 0.0;

  if (vflag == 2) f = update->f_pair;
  else f = atom->f;
  double **x = atom->x;
  double *q = atom->q;
  int *type = atom->type;
  int nlocal = atom->nlocal;
  int nall = atom->nlocal + atom->nghost;
  double *special_coul = force->special_coul;
  double *special_lj = force->special_lj;
  int newton_pair = force->newton_pair;
  double qqrd2e = force->qqrd2e;

  // loop over neighbors of my atoms

  for (i = 0; i < nlocal; i++) {
    qtmp = q[i];
    xtmp = x[i][0];
    ytmp = x[i][1];
    ztmp = x[i][2];
    itype = type[i];
    neighs = neighbor->firstneigh[i];
    numneigh = neighbor->numneigh[i];

    for (k = 0; k < numneigh; k++) {
      j = neighs[k];

      if (j < nall) factor_coul = factor_lj = 1.0;
      else {
	factor_coul = special_coul[j/nall];
	factor_lj = special_lj[j/nall];
	j %= nall;
      }

      delx = xtmp - x[j][0];
      dely = ytmp - x[j][1];
      delz = ztmp - x[j][2];
      rsq = delx*delx + dely*dely + delz*delz;
      jtype = type[j];

      if (rsq < cutsq[itype][jtype]) {
	r2inv = 1.0/rsq;

	if (rsq < cut_coulsq[itype][jtype])
	  forcecoul = qqrd2e * qtmp*q[j]*sqrt(r2inv);
	else forcecoul = 0.0;

	if (rsq < cut_ljsq[itype][jtype]) {
	  rinv = sqrt(r2inv);
	  r3inv = r2inv*rinv;
	  r6inv = r3inv*r3inv;
	  forcelj = r6inv * (lj1[itype][jtype]*r3inv - lj2[itype][jtype]);
	} else forcelj = 0.0;

	fforce = (factor_coul*forcecoul + factor_lj*forcelj) * r2inv;

	f[i][0] += delx*fforce;
	f[i][1] += dely*fforce;
	f[i][2] += delz*fforce;
	if (newton_pair || j < nlocal) {
	  f[j][0] -= delx*fforce;
	  f[j][1] -= dely*fforce;
	  f[j][2] -= delz*fforce;
	}

	if (eflag) {
	  if (newton_pair || j < nlocal) factor = 1.0;
	  else factor = 0.5;
	  if (rsq < cut_coulsq[itype][jtype]) {
	    phicoul = qqrd2e * qtmp*q[j]*sqrt(r2inv);
	    eng_coul += factor*factor_coul*phicoul;
	  }
	  if (rsq < cut_ljsq[itype][jtype]) {
	    philj = r6inv*(lj3[itype][jtype]*r3inv-lj4[itype][jtype]) -
	      offset[itype][jtype];
	    eng_vdwl += factor*factor_lj*philj;
	  }
	}

	if (vflag == 1) {
	  if (newton_pair == 0 && j >= nlocal) fforce *= 0.5;
	  virial[0] += delx*delx*fforce;
	  virial[1] += dely*dely*fforce;
	  virial[2] += delz*delz*fforce;
	  virial[3] += delx*dely*fforce;
	  virial[4] += delx*delz*fforce;
	  virial[5] += dely*delz*fforce;
	}
      }
    }
  }
  if (vflag == 2) virial_compute();
}

/* ----------------------------------------------------------------------
   allocate all arrays 
------------------------------------------------------------------------- */

void PairLJClass2CoulCut::allocate()
{
  allocated = 1;
  int n = atom->ntypes;

  setflag = memory->create_2d_int_array(n+1,n+1,"pair:setflag");
  for (int i = 1; i <= n; i++)
    for (int j = i; j <= n; j++)
      setflag[i][j] = 0;

  cutsq = memory->create_2d_double_array(n+1,n+1,"pair:cutsq");

  cut_lj = memory->create_2d_double_array(n+1,n+1,"pair:cut_lj");
  cut_ljsq = memory->create_2d_double_array(n+1,n+1,"pair:cut_ljsq");
  cut_coul = memory->create_2d_double_array(n+1,n+1,"pair:cut_coul");
  cut_coulsq = memory->create_2d_double_array(n+1,n+1,"pair:cut_coulsq");
  epsilon = memory->create_2d_double_array(n+1,n+1,"pair:epsilon");
  sigma = memory->create_2d_double_array(n+1,n+1,"pair:sigma");
  lj1 = memory->create_2d_double_array(n+1,n+1,"pair:lj1");
  lj2 = memory->create_2d_double_array(n+1,n+1,"pair:lj2");
  lj3 = memory->create_2d_double_array(n+1,n+1,"pair:lj3");
  lj4 = memory->create_2d_double_array(n+1,n+1,"pair:lj4");
  offset = memory->create_2d_double_array(n+1,n+1,"pair:offset");
}

/* ----------------------------------------------------------------------
   global settings 
------------------------------------------------------------------------- */

void PairLJClass2CoulCut::settings(int narg, char **arg)
{
  if (narg < 1 || narg > 2) error->all("Illegal pair_style command");

  cut_lj_global = atof(arg[0]);
  if (narg == 1) cut_coul_global = cut_lj_global;
  else cut_coul_global = atof(arg[1]);

  // reset cutoffs that have been explicitly set

  if (allocated) {
    int i,j;
    for (i = 1; i <= atom->ntypes; i++)
      for (j = i+1; j <= atom->ntypes; j++)
	if (setflag[i][j]) {
	  cut_lj[i][j] = cut_lj_global;
	  cut_coul[i][j] = cut_coul_global;
	}
  }
}

/* ----------------------------------------------------------------------
   set coeffs for one or more type pairs
------------------------------------------------------------------------- */

void PairLJClass2CoulCut::coeff(int narg, char **arg)
{
  if (narg < 4 || narg > 6) error->all("Incorrect args for pair coefficients");
  if (!allocated) allocate();

  int ilo,ihi,jlo,jhi;
  force->bounds(arg[0],atom->ntypes,ilo,ihi);
  force->bounds(arg[1],atom->ntypes,jlo,jhi);

  double epsilon_one = atof(arg[2]);
  double sigma_one = atof(arg[3]);

  double cut_lj_one = cut_lj_global;
  double cut_coul_one = cut_coul_global;
  if (narg >= 5) cut_coul_one = cut_lj_one = atof(arg[4]);
  if (narg == 6) cut_coul_one = atof(arg[5]);

  int count = 0;
  for (int i = ilo; i <= ihi; i++) {
    for (int j = MAX(jlo,i); j <= jhi; j++) {
      epsilon[i][j] = epsilon_one;
      sigma[i][j] = sigma_one;
      cut_lj[i][j] = cut_lj_one;
      cut_coul[i][j] = cut_coul_one;
      setflag[i][j] = 1;
      count++;
    }
  }

  if (count == 0) error->all("Incorrect args for pair coefficients");
}

/* ----------------------------------------------------------------------
   init for one type pair i,j and corresponding j,i
------------------------------------------------------------------------- */

double PairLJClass2CoulCut::init_one(int i, int j)
{
  // always mix epsilon,sigma via sixthpower rules
  // mix distance via user-defined rule

  if (setflag[i][j] == 0) {
    epsilon[i][j] = 2.0 * sqrt(epsilon[i][i]*epsilon[j][j]) *
      pow(sigma[i][i],3.0) * pow(sigma[j][j],3.0) / 
      (pow(sigma[i][i],6.0) + pow(sigma[j][j],6.0));
    sigma[i][j] = 
      pow((0.5 * (pow(sigma[i][i],6.0) + pow(sigma[j][j],6.0))),1.0/6.0);
    cut_lj[i][j] = mix_distance(cut_lj[i][i],cut_lj[j][j]);
    cut_coul[i][j] = mix_distance(cut_coul[i][i],cut_coul[j][j]);
  }

  double cut = MAX(cut_lj[i][j],cut_coul[i][j]);
  cut_ljsq[i][j] = cut_lj[i][j] * cut_lj[i][j];
  cut_coulsq[i][j] = cut_coul[i][j] * cut_coul[i][j];

  lj1[i][j] = 18.0 * epsilon[i][j] * pow(sigma[i][j],9.0);
  lj2[i][j] = 18.0 * epsilon[i][j] * pow(sigma[i][j],6.0);
  lj3[i][j] = 2.0 * epsilon[i][j] * pow(sigma[i][j],9.0);
  lj4[i][j] = 3.0 * epsilon[i][j] * pow(sigma[i][j],6.0);

  if (offset_flag) {
    double ratio = sigma[i][j] / cut_lj[i][j];
    offset[i][j] = epsilon[i][j] * (2.0*pow(ratio,9.0) - 3.0*pow(ratio,6.0));
  } else offset[i][j] = 0.0;

  cut_ljsq[j][i] = cut_ljsq[i][j];
  cut_coulsq[j][i] = cut_coulsq[i][j];
  lj1[j][i] = lj1[i][j];
  lj2[j][i] = lj2[i][j];
  lj3[j][i] = lj3[i][j];
  lj4[j][i] = lj4[i][j];
  offset[j][i] = offset[i][j];

  // compute I,J contribution to long-range tail correction
  // count total # of atoms of type I and J via Allreduce

  if (tail_flag) {
    int *type = atom->type;
    int nlocal = atom->nlocal;

    double count[2],all[2];
    count[0] = count[1] = 0.0;
    for (int k = 0; k < nlocal; k++) {
      if (type[k] == i) count[0] += 1.0;
      if (type[k] == j) count[1] += 1.0;
    }
    MPI_Allreduce(count,all,2,MPI_DOUBLE,MPI_SUM,world);
        
    double PI = 4.0*atan(1.0);
    double sig3 = sigma[i][j]*sigma[i][j]*sigma[i][j];
    double sig6 = sig3*sig3;
    double rc3 = cut_lj[i][j]*cut_lj[i][j]*cut_lj[i][j];
    double rc6 = rc3*rc3;
    etail_ij = 2.0*PI*all[0]*all[1]*epsilon[i][j] *
      sig6 * (sig3 - 3.0*rc3) / (3.0*rc6);
    ptail_ij = 2.0*PI*all[0]*all[1]*epsilon[i][j] * 
      sig6 * (sig3 - 2.0*rc3) / rc6;
  } 

  return cut;
}

/* ----------------------------------------------------------------------
   init specific to this pair style
------------------------------------------------------------------------- */

void PairLJClass2CoulCut::init_style()
{
  if (atom->q == NULL)
    error->all("Pair style lj/class2/coul/cut requires atom attribute q");
}

/* ----------------------------------------------------------------------
   proc 0 writes to restart file 
------------------------------------------------------------------------- */

void PairLJClass2CoulCut::write_restart(FILE *fp)
{
  write_restart_settings(fp);

  int i,j;
  for (i = 1; i <= atom->ntypes; i++)
    for (j = i; j <= atom->ntypes; j++) {
      fwrite(&setflag[i][j],sizeof(int),1,fp);
      if (setflag[i][j]) {
	fwrite(&epsilon[i][j],sizeof(double),1,fp);
	fwrite(&sigma[i][j],sizeof(double),1,fp);
	fwrite(&cut_lj[i][j],sizeof(double),1,fp);
	fwrite(&cut_coul[i][j],sizeof(double),1,fp);
      }
    }
}

/* ----------------------------------------------------------------------
   proc 0 reads from restart file, bcasts
------------------------------------------------------------------------- */

void PairLJClass2CoulCut::read_restart(FILE *fp)
{
  read_restart_settings(fp);
  allocate();

  int i,j;
  int me = comm->me;
  for (i = 1; i <= atom->ntypes; i++)
    for (j = i; j <= atom->ntypes; j++) {
      if (me == 0) fread(&setflag[i][j],sizeof(int),1,fp);
      MPI_Bcast(&setflag[i][j],1,MPI_INT,0,world);
      if (setflag[i][j]) {
	if (me == 0) {
	  fread(&epsilon[i][j],sizeof(double),1,fp);
	  fread(&sigma[i][j],sizeof(double),1,fp);
	  fread(&cut_lj[i][j],sizeof(double),1,fp);
	  fread(&cut_coul[i][j],sizeof(double),1,fp);
	}
	MPI_Bcast(&epsilon[i][j],1,MPI_DOUBLE,0,world);
	MPI_Bcast(&sigma[i][j],1,MPI_DOUBLE,0,world);
	MPI_Bcast(&cut_lj[i][j],1,MPI_DOUBLE,0,world);
	MPI_Bcast(&cut_coul[i][j],1,MPI_DOUBLE,0,world);
      }
    }
}

/* ----------------------------------------------------------------------
   proc 0 writes to restart file
------------------------------------------------------------------------- */

void PairLJClass2CoulCut::write_restart_settings(FILE *fp)
{
  fwrite(&cut_lj_global,sizeof(double),1,fp);
  fwrite(&cut_coul_global,sizeof(double),1,fp);
  fwrite(&offset_flag,sizeof(int),1,fp);
  fwrite(&mix_flag,sizeof(int),1,fp);
}

/* ----------------------------------------------------------------------
   proc 0 reads from restart file, bcasts
------------------------------------------------------------------------- */

void PairLJClass2CoulCut::read_restart_settings(FILE *fp)
{
  if (comm->me == 0) {
    fread(&cut_lj_global,sizeof(double),1,fp);
    fread(&cut_coul_global,sizeof(double),1,fp);
    fread(&offset_flag,sizeof(int),1,fp);
    fread(&mix_flag,sizeof(int),1,fp);
  }
  MPI_Bcast(&cut_lj_global,1,MPI_DOUBLE,0,world);
  MPI_Bcast(&cut_coul_global,1,MPI_DOUBLE,0,world);
  MPI_Bcast(&offset_flag,1,MPI_INT,0,world);
  MPI_Bcast(&mix_flag,1,MPI_INT,0,world);
}

/* ---------------------------------------------------------------------- */

void PairLJClass2CoulCut::single(int i, int j, int itype, int jtype,
				 double rsq, double factor_coul,
				 double factor_lj, int eflag, One &one)
{
  double r2inv,rinv,r3inv,r6inv,forcecoul,forcelj,phicoul,philj;

  r2inv = 1.0/rsq;
  if (rsq < cut_coulsq[itype][jtype])
    forcecoul = force->qqrd2e * atom->q[i]*atom->q[j]*sqrt(r2inv);
  else forcecoul = 0.0;
  if (rsq < cut_ljsq[itype][jtype]) {
    rinv = sqrt(r2inv);
    r3inv = r2inv*rinv;
    r6inv = r3inv*r3inv;
    forcelj = r6inv * (lj1[itype][jtype]*r3inv - lj2[itype][jtype]);
  } else forcelj = 0.0;
  one.fforce = (factor_coul*forcecoul + factor_lj*forcelj) * r2inv;

  if (eflag) {
    if (rsq < cut_coulsq[itype][jtype]) {
      phicoul = force->qqrd2e * atom->q[i]*atom->q[j]*sqrt(r2inv);
      one.eng_coul = factor_coul*phicoul;
    } else one.eng_coul = 0.0;
    if (rsq < cut_ljsq[itype][jtype]) {
      philj = r6inv*(lj3[itype][jtype]*r3inv-lj4[itype][jtype]) -
	offset[itype][jtype];
      one.eng_vdwl = factor_lj*philj;
    } else one.eng_vdwl = 0.0;
  }
}
