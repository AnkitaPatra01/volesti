// [[Rcpp::depends(BH)]]

// VolEsti (volume computation and sampling library)

// Copyright (c) 20012-2020 Vissarion Fisikopoulos
// Copyright (c) 2018-2020 Apostolos Chalkis

//Contributed and/or modified by Apostolos Chalkis, as part of Google Summer of Code 2018 program.
//Contributed and/or modified by Alexandros Manochis, as part of Google Summer of Code 2020 program.

#include <Rcpp.h>
#include <RcppEigen.h>
#include <chrono>

#include <boost/random.hpp>
#include <boost/random/uniform_int.hpp>
#include <boost/random/normal_distribution.hpp>
#include <boost/random/uniform_real_distribution.hpp>
#include "cartesian_geom/cartesian_kernel.h"
#include "convex_bodies/hpolytope.h"
#include "random_walks/random_walks.hpp"
//#include "volume/volume_sequence_of_balls.hpp"
//#include "volume/volume_cooling_gaussians.hpp"
//#include "preprocess/min_sampling_covering_ellipsoid_rounding.hpp"
//#include "preprocess/svd_rounding.hpp"
//#include "preprocess/max_inscribed_ellipsoid_rounding.hpp"
#include "extractMatPoly.h"
#include "sampling/sampling_multi.hpp"


//' Internal rcpp function for the rounding of a convex polytope
//'
//' @param P A convex polytope (H- or V-representation or zonotope).
//' @param method Optional. The method to use for rounding, a) \code{'min_ellipsoid'} for the method based on mimimmum volume enclosing ellipsoid of a uniform sample from P, b) \code{'max_ellipsoid'} for the method based on maximum volume enclosed ellipsoid in P, (c) \code{'svd'} for the method based on svd decomposition. The default method is \code{'min_ellipsoid'} for all the representations.
//' @param seed Optional. A fixed seed for the number generator.
//'
//' @keywords internal
//'
//' @return A numerical matrix that describes the rounded polytope, a numerical matrix of the inverse linear transofmation that is applied on the input polytope, the numerical vector the the input polytope is shifted and the determinant of the matrix of the linear transformation that is applied on the input polytope.

//MT T;
 //   VT T_shift;
//    unsigned int num_rounding_steps;                                
//    bool fail;
//    bool converged;
//    bool last_round_under_p;
//    NT max_s;
//    NT prev_max_s;
//    unsigned int round_it;

// [[Rcpp::export]]
Rcpp::List mmcs_phase(Rcpp::NumericVector center, double radius,
                              Rcpp::List parameters){

    typedef double NT;
    typedef Cartesian<NT>    Kernel;
    typedef typename Kernel::Point    Point;
    typedef BoostRandomNumberGenerator<boost::mt19937, NT> RNGType;
    typedef HPolytope<Point> Hpolytope;
    typedef Eigen::Matrix<NT,Eigen::Dynamic,1> VT;
    typedef Eigen::Matrix<NT,Eigen::Dynamic,Eigen::Dynamic> MT;

    MT T = Rcpp::as<MT>(parameters["T"]);
    VT T_shift = Rcpp::as<VT>(parameters["T_shift"]);

    unsigned int round_it = parameters["round_it"], num_rounding_steps = parameters["num_rounding_steps"], 
        walk_length = parameters["walk_length"], num_its = 20, Neff = parameters["Neff"], 
        window = parameters["window"], max_num_samples = parameters["max_num_samples"], total_samples;
    NT max_s, L = parameters["L"], s_cutoff = 4.0;
    bool complete = parameters["complete"], request_rounding = parameters["request_rounding"],
         rounding_completed = parameters["rounding_completed"];
    bool req_round_temp = request_rounding;
    if (request_rounding && rounding_completed) {
        req_round_temp = false;
    }
    
 
    std::pair<Point, NT> InnerBall;
    InnerBall.first = Point(Rcpp::as<VT>(center));
    InnerBall.second = radius;

    Hpolytope P(Rcpp::as<MT>(parameters["A"]).cols(), Rcpp::as<MT>(parameters["A"]), Rcpp::as<VT>(parameters["b"]));
    P.set_InnerBall(InnerBall);
    P.normalize();

    int n = P.dimension(), nburns;
    
    if (req_round_temp) {
        nburns = num_rounding_steps / window + 1;
    } else {
        nburns = max_num_samples / window + 1;
    }

    //std::cout<<"round_it = "<<round_it<<std::endl;
    //std::cout<<"center = "<<Rcpp::as<VT>(center).transpose()<<std::endl;
    //std::cout<<"radius = "<<radius<<std::endl;
    //std::cout<<"num_rounding_steps = "<<num_rounding_steps<<std::endl;
    //std::cout<<"nburns = "<<nburns<<std::endl;
    //std::cout<<"L = "<<L<<std::endl;

    //typedef BoostRandomNumberGenerator<boost::mt19937, NT> RNGType;
    RNGType rng(n);

    AcceleratedSpeedpBilliardWalk WalkType(L);
    unsigned int Neff_sampled;
    MT TotalRandPoints;
    uniform_sampling_speedup(P, rng, walk_length, Neff, max_num_samples, window, 
                             Neff_sampled, total_samples, num_rounding_steps, TotalRandPoints,
                             complete, InnerBall.first.getCoefficients(), 
                             nburns, req_round_temp, WalkType); //num_sampled TODO!

                             /*Polytope &P,
                   RandomNumberGenerator &rng,
                   const unsigned int &walk_len,
                   const unsigned int &Neff,
                   unsigned int const& max_num_samples,
                   unsigned int &window,
                   unsigned int &Neff_sampled,
                   unsigned int const& num_rounding_steps,
                   MT &TotalRandPoints,
                   bool &complete,
                   const VT &starting_point,
                   unsigned int const& nburns,
                   bool request_rounding,
                   WalkTypePolicy &WalkType*/

    int N = TotalRandPoints.rows();
    MT Samples = TotalRandPoints.transpose(); //do not copy TODO!
    //std::cout<<"TotalRandPoints.rows() = "<<TotalRandPoints.rows()<<", TotalRandPoints.cols() = "<<TotalRandPoints.cols()<<std::endl;
    if (!complete) {
        if (request_rounding && !rounding_completed) {
            VT shift(n), s(n);
            MT V(n,n), S(n,n), round_mat;
            for (int i = 0; i < P.dimension(); ++i) {
                shift(i) = TotalRandPoints.col(i).mean();
            }

            for (int i = 0; i < N; ++i) {
                TotalRandPoints.row(i) = TotalRandPoints.row(i) - shift.transpose();
            }

            Eigen::BDCSVD<MT> svd(TotalRandPoints, Eigen::ComputeFullV);
            s = svd.singularValues() / svd.singularValues().minCoeff();

            if (s.maxCoeff() >= 2.0) {
                for (int i = 0; i < s.size(); ++i) {
                    if (s(i) < 2.0) {
                        s(i) = 1.0;
                    }
                }
                V = svd.matrixV();
            } else {
                s = VT::Ones(P.dimension());
                V = MT::Identity(P.dimension(), P.dimension());
            }
            max_s = s.maxCoeff();
            //std::cout<<"max_s = "<<max_s<<std::endl;
            S = s.asDiagonal();
            round_mat = V * S;
            //r_inv = VT::Ones(n).cwiseProduct(s.cwiseInverse()).asDiagonal() * V.transpose();

            round_it++;
            //prev_max_s = max_s;

            P.shift(shift);
            P.linear_transformIt(round_mat);
            //InnerBall = P.ComputeInnerBall();
            T_shift += T * shift;
            T = T * round_mat;

            if (max_s <= s_cutoff || round_it > num_its) {
                rounding_completed = true;
            }
        }
    }

    return Rcpp::List::create(Rcpp::Named("A") = Rcpp::wrap(P.get_mat()),
                              Rcpp::Named("b") = Rcpp::wrap(P.get_vec()),
                              Rcpp::Named("T") = Rcpp::wrap(T),
                              Rcpp::Named("T_shift") = Rcpp::wrap(T_shift),
                              Rcpp::Named("round_it") = round_it,
                              Rcpp::Named("num_rounding_steps") = num_rounding_steps,
                              Rcpp::Named("walk_length") = walk_length,
                              Rcpp::Named("L") = L,
                              Rcpp::Named("window") = window,
                              Rcpp::Named("Neff") = Neff,
                              Rcpp::Named("Neff_sampled") = Neff_sampled,
                              Rcpp::Named("total_samples") = total_samples,
                              Rcpp::Named("request_rounding") = request_rounding,
                              Rcpp::Named("correlated_samples") = Rcpp::wrap(Samples),
                              Rcpp::Named("complete") = complete,
                              Rcpp::Named("request_rounding") = request_rounding,
                              Rcpp::Named("max_num_samples") = max_num_samples,
                              Rcpp::Named("rounding_completed") = rounding_completed);

}


