/*
 *  _____ _____ ________  _
 * /  __//  __//  __/\  \//
 * | |  _|  \  |  \   \  / 
 * | |_//|  /_ |  /_  /  \ 
 * \____\\____\\____\/__/\\
 *
 * Graphics Environment for EXperimentations.
 *  Copyright (C) 2006 INRIA - Project ALICE
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *  If you modify this software, you should include a notice giving the
 *  name of the person performing the modification, the date of modification,
 *  and the reason for such modification.
 *
 *  Contact: 
 *
 *     ALICE Project - INRIA
 *     INRIA Lorraine, 
 *     Campus Scientifique, BP 239
 *     54506 VANDOEUVRE LES NANCY CEDEX 
 *     FRANCE
 *
 *  Note that the GNU General Public License does not permit incorporating
 *  the Software into proprietary programs. 
 */

#ifndef __DELAUNAY_CVT__
#define __DELAUNAY_CVT__

#include "delaunay.h"
#include "lloyd_funcs.h"
#include <Geex/numerics/optimizer.h>
#include <Geex/numerics/lbfgs_interface.h>
#include <GL/gl.h>

namespace Geex {

    enum CVTMode { LLOYD, NEWTON, THETA } ;
#define CVTModeNames "LLOYD" | "NEWTON" | "THETA" 

    enum AnisoMode { CONSTANT, R_INV, BORDER } ;
#define AnisoModeNames "Const" | "1/R" | "Border" 

    enum QuasiCenterMode {CENTROID, QUASI_INCENTER};

// 	enum PVDMode { NO_COPY, FULL_COPY, MIN_COPY } ;
// #define PVDModeNames "NO_COPY | Full Copy" | "Min Copy"

    class DelaunayCVT {
    public:
        enum { MAX_P = 9 } ;
        DelaunayCVT(Delaunay* delaunay) ;
        ~DelaunayCVT() ;
        void lloyd(int nb_iter = 1, bool redraw = true) ;
        void newton_lloyd(int nb_iter = 1, bool redraw = true) ;
        static DelaunayCVT* instance() { return instance_ ; }
        std::vector<Delaunay::Vertex_handle>& all_vertices() { return delaunay_->all_vertices_ ; }

		void lloyd_fpo(int nb_iter = 1, bool redraw = true) ;

        double default_theta(const vec2& V) ;
        double default_theta(Delaunay::Vertex_handle v) ;
        double default_rho(Delaunay::Vertex_handle v) ;

        void set_vertices(const double* x) ;        
		void move_vertices(vec2 delta) ;
        void get_cell_centroid(Delaunay::Vertex_handle v, vec2& g, double& V) ;
		void get_boundary_cell_centroid(Delaunay::Vertex_handle v, vec2& g, double& V) ;
		//void get_primary_position(vec2& g) ;
		void get_cell_primary_centroid(Delaunay::Vertex_handle v, vec2& g, double& V) ;
        void get_cell_quasi_incenter(Delaunay::Vertex_handle v, vec2& g);
        bool get_fg(Delaunay::Vertex_handle v, double& f, vec2& grad_f) ;
		bool get_fgv(Delaunay::Vertex_handle v, double& f, vec2& grad_f, double& area) ;
        double lloyd_energy() ;

		// ---- laplacian smoothing
		void smooth(int nb_iter = 1, bool redrew = true) ;
		void get_vertex_center(Delaunay::Vertex_handle v, vec2& g) ;
		// ---- build the Hessian matrix of current configuration
		void compute_hessian() ;

        float& Lp() { return Lp_ ; }
        GLenum& aniso_mode() { return aniso_mode_ ; }
        float& Xscale() { return X_scale_ ; }
        float& Yscale() { return Y_scale_ ; }
// 		GLenum& pvd_mode() { return pvd_mode_ ; }
// 		GLboolean period() { return pvd_mode_ != NO_COPY ; } //dxy test
		
        GLenum& center_mode() { return center_mode_; }


        void begin_func(int i, int j) {
            cur_func_ = funcs_->get_template(i,j) ;
            cur_func_->begin_xp() ;
            cur_P_ = 0 ;
        }

        void begin_func_regular() {
            cur_func_ = funcs_->get_regular_template() ;
            cur_func_->begin_xp() ;
            cur_P_ = 0 ;
        }

        void end_func() {
            cur_func_->end_xp() ;
            cur_func_->eval() ;
        }

        void new_P(Delaunay::Vertex_handle v) {
            gx_assert(v->index >= 0) ;
            iP[cur_P_] = v->index ;
            cur_P_++ ;
            cur_func_->add_x(v->point().x()) ;
            cur_func_->add_x(v->point().y()) ;
        }

        void new_Q(const Line<real>& q) {
            cur_func_->add_p(q.a) ;
            cur_func_->add_p(q.b) ;
            cur_func_->add_p(q.c) ;
        }

        void new_R(const vec2& r) {
            cur_func_->add_p(r.x) ;
            cur_func_->add_p(r.y) ;
        }

        void set_anisotropy(const vec2& X, const vec2& Y) ;
        void set_anisotropy(const vec2& P) ;
        void set_anisotropy(Delaunay::Vertex_handle V) ;
        
        void query_anisotropy(const vec2& P, vec2& U, vec2& V) ;

        int get_vertex_config(PolygonVertex& V) {
            if(V.on_boundary) { return 0 ; }
            int nb_P = V.bisectors.size() ;
            int nb_Q = V.boundary_edges.size() ;
            gx_assert(nb_P + nb_Q >= 2) ;
            gx_assert(nb_P <= 2) ;
            return nb_P ;
        }

        void get_PQR(PolygonVertex& V, int center_index) ;


        void add_to_fg(double& f, double* g) ;
        void funcgrad(const double* x, double& f, double* g, bool& valid) ;
        void funcgrad_simple(const double* x, double& f, double* g, bool& valid) ;
        void funcgrad_symbolic(const double* x, double& f, double* g, bool& valid) ;

        GLenum& mode() { return mode_ ; }

        Delaunay* delaunay() { return delaunay_ ; }

		GLboolean& snap_boundary() { return snap_boundary_ ; }

		///dxy add: direction core
		void get_dual_edge_direction_fg(const vec2& v, const vec2& v1, const vec2& v2, double& f, vec2& grad_f); 
		void get_vertex_direction_fg(Delaunay::Vertex_handle v, double& f, vec2& grad_f);
		void get_vertex_lloyd_fg(Delaunay::Vertex_handle v, double& f, vec2& grad_f);
		//
		void get_field(const vec2& v, double& angle);
		///

		//dxy add: density
		double get_density(const vec2 &v) { //
			vec2 r(v.x-floor(v.x), v.y-floor(v.y));  //translate to [0,1]x[0,1]
			vec2 center(0.5, 0.5);
			vec2 p = r-center;
			return exp(-15 * p.x * p.x - 15 * p.y * p.y);
		}
		//

		///dxy add: getter
		GLboolean&  use_density()  { return use_density_; }
		GLboolean&  use_lloyd_energy() { return use_lloyd_energy_; }
		GLboolean&  direction_field() { return direction_field_; }
		/*GLboolean&  direction_constrained_edge() { return direction_constrained_edge_; }
		GLboolean&  direction_boundary_edge() { return direction_boundary_edge_; }
		GLboolean&  direction_gradient() { return direction_gradient_; }*/
		GLboolean&  use_quad_field() { return use_quad_field_; }
		GLboolean&  use_topo_optimization() { return use_topo_optimization_ ; }
		GLboolean&  use_balanced_stretch() { return use_balanced_stretch_; }
		float& direction_angle() { return direction_angle_; }
		float& direction_factor() { return direction_factor_; }
		float& quad_factor() { return quad_factor_; }

		enum DirectionEnergyMode {ANGLE, ANGLE_DUAL, ANGLE_PRIMAL };
		enum DirectionFieldMode { F_CONSTANT, F_CIRCLE, F_SIN };
		enum DirectionEdgeWeightMode { W_DUAL_LENGTH, W_LLOYD_ENERGY };
		GLenum& direction_energy_mode() { return direction_energy_mode_; }
		GLenum& direction_field_mode() { return direction_field_mode_; }
		GLenum& direction_edge_weight_mode() { return direction_edge_weight_mode_; }

		int& num_Pair57_move() { return num_Pair57_move_; }
		int& num_Pair57_smoothing() { return num_Pair57_smoothing_; }

		GLboolean& use_auto_save() { return use_auto_save_; }
		///


    private:
        Delaunay* delaunay_ ;
        LloydFuncs* funcs_ ;
        LloydFuncs* funcs_table_[MAX_P+1] ;
        LloydFuncs* funcs_table_theta_[MAX_P+1] ;
        Function* cur_func_ ;
        int cur_P_ ; 
        int iP[30] ;

        float Lp_ ;
        bool symbolic_ ;
        GLenum aniso_mode_ ;
        float X_scale_ ;
        float Y_scale_ ;
        static DelaunayCVT* instance_ ;

        GLenum center_mode_;
		//GLenum pvd_mode_ ; //dxy test
//		GLboolean period_ ;

        GLenum mode_ ;
        GLboolean use_theta_ ;

        GLboolean insert_boundary_ ;
		GLboolean snap_boundary_ ;

		///dxy add
		GLboolean use_density_;
		GLboolean use_lloyd_energy_;
		GLboolean direction_field_;
		/*GLboolean direction_constrained_edge_;
		GLboolean direction_boundary_edge_;
		GLboolean direction_gradient_;*/
		GLboolean use_quad_field_;
		GLboolean use_topo_optimization_;
		GLboolean use_balanced_stretch_;
		float direction_angle_;
		float direction_factor_;
		float quad_factor_;
		GLenum direction_energy_mode_;
		GLenum direction_field_mode_;
		GLenum direction_edge_weight_mode_;
		//Pair57 move
		int num_Pair57_move_;
		int num_Pair57_smoothing_;
		//auto save pts
		GLboolean use_auto_save_;
		///
    } ;
    

}

#endif
