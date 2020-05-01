/* ------------------------------------------------------------------
    TILD - Theoretically Informed Langevan Dynamics
    This replicates the TILD coe done by the Riggleman group, 
    previously known as Dynamical Mean Field Theory. 
    
    Copyright (2019) Christian Tabedzki and Zachariah Vicars.
    tabedzki@seas.upenn.edu zvicars@seas.upenn.edu
-------------------------------------------------------------------- */

#ifdef KSPACE_CLASS

KSpaceStyle(tild,TILD)

#else

#ifndef LMP_TILD_H
#define LMP_TILD_H

#include "kspace.h"
#include <tuple>
#include <unordered_map>
#include <vector>
#include <utility>      
#include <bits/stdc++.h> 
#include <algorithm>

namespace LAMMPS_NS {

class TILD : public KSpace{
 public:
  TILD (class LAMMPS *);
  virtual ~TILD();
  virtual void init();
  virtual void setup();
  virtual void settings(int, char **);
  virtual void compute(int, int);
  void precompute_density_fft();
  double memory_usage();
  class FFT3d *fft1,*fft2;
  class Remap *remap;
  class GridComm *cg;
  class GridComm *cg_peratom;
  int *groupbits;
  double a_squared;
  void setup_grid();
  // virtual int timing_1d(int, double &);
  // virtual int timing_3d(int, double &);

  void field_groups(int);
  void field_gradient(FFT_SCALAR*, FFT_SCALAR**, int);
  // void get_k_alias(int, double *);

  void write_restart(FILE *);
  void read_restart(FILE *);
  void write_restart_settings(FILE *);
  void read_restart_settings(FILE *);
  void write_data(FILE *);
  void write_data_all(FILE *);

 protected:
  // For future kspace_hybrid
  int nstyles;                  // # of sub-styles For future kspace_hybrid
  int **setflag;                 // 0/1 = whether each i,j has been set

  FFT_SCALAR *temp;
  FFT_SCALAR ***grad_potent, ***grad_potent_hat, **potent, **potent_hat;
  int **potent_map;
  FFT_SCALAR *****gradWtype;
  int kxmax,kymax,kzmax;
  int kcount,kmax,kmax3d,kmax_created;
  double gsqmx,volume;
  int nmax;
  void complex_multiply(FFT_SCALAR*,FFT_SCALAR*,FFT_SCALAR*, int);
  void complex_multiply(double*,double*, int);
  double **potent_param;
  int npot, *pot_map;
  int ***potent_type_map;
  double rho0;
  double set_rho0, old_volume;

  double unitk[3];
  int *kxvecs,*kyvecs,*kzvecs;
  int kxmax_orig,kymax_orig,kzmax_orig;
  double *ug;
  int dim;
  double **eg;
  double ***vg;
  FFT_SCALAR ***vg_hat;
  //double **eg,**vg;
  double **ek;
  double *sfacrl,*sfacim,*sfacrl_all,*sfacim_all;
  double ***cs,***sn;
  int *assigned_pot, *potent_to_compressed;
  int **group_with_potential;
  int factorable(int);
  double **chi;
  double **a2;
  double **rp;
  double **xi;
  double grid_res;
  virtual int modify_param(int, char**);
  int num_potent;
  double calculate_rho0();
  int get_style(const int, const int);

  // group-group interactions

  int group_allocate_flag;
  double *sfacrl_A,*sfacim_A,*sfacrl_A_all,*sfacim_A_all;
  double *sfacrl_B,*sfacim_B,*sfacrl_B_all,*sfacim_B_all;

  double rms(int, double, bigint, double);
  // virtual void eik_dot_r();
  void coeffs();
  virtual void allocate();
  void deallocate();
  void slabcorr();
  void init_gauss();
  void init_potential(FFT_SCALAR*, const int, const double*);
  void init_potential_ft(FFT_SCALAR*, const int, const double*);
  void calc_work(double*, const int, const int);
  //void init_potential(FFT_SCALAR*, std::tuple<int,int,std::vector<double>> &tup);
  void init_cross_potentials();
  double get_k_alias(int, double*);
  void get_k_alias(FFT_SCALAR *, FFT_SCALAR **);



 protected:
  int me,nprocs;
  int nfactors;
  int *factors;
  double cutoff;
  double kappa, w_3body;
  double delxinv,delyinv,delzinv,delvolinv;
  double h_x,h_y,h_z;
  double shift,shiftone;
  int peratom_allocate_flag;

  int nxlo_in,nylo_in,nzlo_in,nxhi_in,nyhi_in,nzhi_in;
  int nxlo_out,nylo_out,nzlo_out,nxhi_out,nyhi_out,nzhi_out;
  int nxlo_ghost,nxhi_ghost,nylo_ghost,nyhi_ghost,nzlo_ghost,nzhi_ghost;
  int nxlo_fft,nylo_fft,nzlo_fft,nxhi_fft,nyhi_fft,nzhi_fft;
  int nlower,nupper;
  int ngrid,nfft,nfft_both;
  int set_rho0_flag, subtract_rho0, normalize_by_rho0, mix_flag, sub_flag, norm_flag;
  int *total_counter, specified_all_group, start_group_ind, total_groups; 

  FFT_SCALAR ***density_brick;
  FFT_SCALAR ***vdx_brick,***vdy_brick,***vdz_brick;
  FFT_SCALAR ***u_brick;
  FFT_SCALAR ***v0_brick,***v1_brick,***v2_brick;
  FFT_SCALAR ***v3_brick,***v4_brick,***v5_brick;
  double *greensfn;
  double *fkx,*fky,*fkz;
  double *fkx2, *fky2, *fkz2;
  FFT_SCALAR *density_fft;
  FFT_SCALAR *work1,*work2;
  FFT_SCALAR *worki,*workj;

  double *gf_b;
  FFT_SCALAR **rho1d,**rho_coeff,**drho1d,**drho_coeff;
  double *sf_precoeff1, *sf_precoeff2, *sf_precoeff3;
  double *sf_precoeff4, *sf_precoeff5, *sf_precoeff6;
  double sf_coeff[6];          // coefficients for calculating ad self-forces
  double **acons;

  // group-group interactions

  FFT_SCALAR ***density_A_brick,***density_B_brick;
  FFT_SCALAR *density_A_fft,*density_B_fft;

  int **part2grid;             // storage for particle -> grid mapping

  double *boxlo;
                               // TIP4P settings
  int typeH,typeO;             // atom types of TIP4P water H and O atoms
  double qdist;                // distance from O site to negative charge
  double alpha;                // geometric factor

  void set_grid_global();
  void set_grid();
  void set_grid_6();
  void set_init_g6();
  void set_n_pppm_6();
  void calc_csum();
//   void set_grid_local();
  void adjust_gewald();
  double newton_raphson_f();
  double derivf();
  double final_accuracy();

  virtual void allocate_peratom();
  virtual void deallocate_peratom();
  double estimate_ik_error(double, double, bigint);
  // virtual double compute_qopt();
  // virtual double compute_qopt_ik();
  // virtual double compute_qopt_ad();
  // virtual void compute_gf_denom();
  // virtual void compute_gf_ik();
  // virtual void compute_gf_ad();
  // void compute_sf_precoeff();

  virtual void particle_map(double, double, double,
                             double, int **, int, int,
                             int, int, int,
                             int, int, int);
  // virtual void make_rho();
  virtual void make_rho_none();
  virtual void brick2fft(int, int, int, int, int, int,
                         FFT_SCALAR ***, FFT_SCALAR *, FFT_SCALAR *,
                         LAMMPS_NS::Remap *);
  virtual void brick2fft();
  virtual void brick2fft_none();
  void ev_calculation(double*, double*, double*, int, int);
  //void ev_calculation();


  // virtual void poisson();
  // virtual void poisson_ik();
  // virtual void poisson_ad();

  // virtual void fieldforce();
  virtual void fieldforce_param();
  // virtual void fieldforce_ik();
  // virtual void fieldforce_ad();

  // virtual void poisson_peratom();
  // virtual void fieldforce_peratom();
  void procs2grid2d(int,int,int,int *, int*);
  void compute_rho1d(const FFT_SCALAR &, const FFT_SCALAR &,
                     const FFT_SCALAR &);
  void compute_drho1d(const FFT_SCALAR &, const FFT_SCALAR &,
                     const FFT_SCALAR &);
  void compute_rho_coeff();

  // grid communication

  virtual void pack_forward(int, FFT_SCALAR *, int, int *);
  virtual void unpack_forward(int, FFT_SCALAR *, int, int *);
  virtual void pack_reverse(int, FFT_SCALAR *, int, int *);
  virtual void unpack_reverse(int, FFT_SCALAR *, int, int *);

  // triclinic

  int triclinic;               // domain settings, orthog or triclinic
  void setup_triclinic();
  // void compute_gf_ik_triclinic();
  // void poisson_ik_triclinic();
  // void poisson_groups_triclinic();

  // group-group interactions

  virtual void allocate_groups();
  virtual void deallocate_groups();
  // virtual void make_rho_groups(int, int, int);
  // virtual void poisson_groups(int);
  // virtual void slabcorr_groups(int,int,int);
  // virtual void make_rho_none();

  // group-group interactions

  void set_fft_parameters(int&, int&, int&, int&, int&,int&,
                          int&, int&,int&, int&, int&,int&,
                          int&, int&,int&, int&, int&,int&,
                          int&, int&,int&, int&, int&,
                          int&, int&, int&,
                          double&, double&, int&);
  void accumulate_gradient();
  void force_field_grad();
  void vir_func_init();

  FFT_SCALAR ****density_brick_types;
  FFT_SCALAR *kappa_density;
  FFT_SCALAR **density_fft_types;
  FFT_SCALAR **density_of_types_fft_ed;
  FFT_SCALAR **density_of_potentials_fft_ed;
  FFT_SCALAR *ktmp;
  FFT_SCALAR *ktmpi;
  FFT_SCALAR *ktmpj;
  FFT_SCALAR *ktmp2;
  FFT_SCALAR *ktmp2i;
  FFT_SCALAR *ktmp2j;
  FFT_SCALAR *tmp;
  void compute_rho1d(const FFT_SCALAR &, const FFT_SCALAR &,
                     const FFT_SCALAR &, int, FFT_SCALAR **, FFT_SCALAR **);
  void compute_rho_coeff(FFT_SCALAR **,FFT_SCALAR **, int);

  double csumij;
  double csum;
  double *B;
  double *csumi;  //needed as correction term for per atom calculations!
  double *cii;
  int csumflag;
  int nsplit;
  int nsplit_alloc;
  int nxlo_in_6,nylo_in_6,nzlo_in_6,nxhi_in_6,nyhi_in_6,nzhi_in_6;
  int nxlo_out_6,nylo_out_6,nzlo_out_6,nxhi_out_6,nyhi_out_6,nzhi_out_6;
  int nxlo_fft_6,nylo_fft_6,nzlo_fft_6,nxhi_fft_6,nyhi_fft_6,nzhi_fft_6;
  int nlower_6,nupper_6;
  int ngrid_6,nfft_6,nfft_both_6;
  double compute_qopt_6();
  double compute_qopt_6_ik();
  double compute_qopt_6_ad();

};

}

#endif
#endif

