// [[Rcpp::depends(BH)]]

// VolEsti (volume computation and sampling library)

// Copyright (c) 2012-2020 Vissarion Fisikopoulos
// Copyright (c) 2018-2020 Apostolos Chalkis

//Contributed and/or modified by Apostolos Chalkis, as part of Google Summer of Code 2018 and 2019 program.

// Contributed and/or modified by Alexandros Manochis, as part of Google Summer of Code 2020 program.

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
#include "sampling/sampling.hpp"
//#include "ode_solvers/ode_solvers.hpp"
//#include "ode_solvers/oracle_functors_rcpp.hpp"


enum random_walks {
  ball_walk,
  rdhr,
  cdhr,
  billiard,
  accelarated_billiard,
  dikin_walk,
  vaidya_walk,
  john_walk,
  brdhr,
  bcdhr,
  hmc,
  uld
};

template <
        typename Polytope,
        typename RNGType,
        typename PointList,
        typename NT,
        typename Point
>
void sample_from_polytope(Polytope &P, int type, RNGType &rng, PointList &randPoints,
        unsigned int const& walkL, unsigned int const& numpoints,
        bool const& gaussian, NT const& a, NT const& L,
        Point const& StartingPoint, unsigned int const& nburns,
        bool const& set_L, random_walks walk)
{
    switch (walk)
    {
    case bcdhr:
        uniform_sampling_boundary <BCDHRWalk>(randPoints, P, rng, walkL, numpoints,
                     StartingPoint, nburns);
        break;
    case brdhr:
        uniform_sampling_boundary <BRDHRWalk>(randPoints, P, rng, walkL, numpoints,
                     StartingPoint, nburns);
        break;
    case cdhr:
        if(gaussian) {
            gaussian_sampling<GaussianCDHRWalk>(randPoints, P, rng, walkL, numpoints,
                                             a, StartingPoint, nburns);
        } else {
            uniform_sampling<CDHRWalk>(randPoints, P, rng, walkL, numpoints,
                                             StartingPoint, nburns);
        }
        break;
    case rdhr:
        if(gaussian) {
            gaussian_sampling<GaussianRDHRWalk>(randPoints, P, rng, walkL, numpoints,
                                             a, StartingPoint, nburns);
        } else {
            uniform_sampling<RDHRWalk>(randPoints, P, rng, walkL, numpoints,
                                             StartingPoint, nburns);
        }
        break;
    case vaidya_walk:
        if (set_L) {
            VaidyaWalk WalkType(L);
            uniform_sampling(randPoints, P, rng, WalkType, walkL, numpoints, StartingPoint, nburns);
        } else {
            uniform_sampling<VaidyaWalk>(randPoints, P, rng, walkL, numpoints,
                                     StartingPoint, nburns);
        }
        break;
    case dikin_walk:
        if (set_L) {
            DikinWalk WalkType(L);
            uniform_sampling(randPoints, P, rng, WalkType, walkL, numpoints, StartingPoint, nburns);
        } else {
            uniform_sampling<DikinWalk>(randPoints, P, rng, walkL, numpoints,
                                     StartingPoint, nburns);
        }
        break;
    case john_walk:
        if (set_L) {
            JohnWalk WalkType(L);
            uniform_sampling(randPoints, P, rng, WalkType, walkL, numpoints, StartingPoint, nburns);
        } else {
            uniform_sampling<JohnWalk>(randPoints, P, rng, walkL, numpoints,
                                    StartingPoint, nburns);
        }
        break;
    case billiard:
        if(set_L) {
            BilliardWalk WalkType(L);
            uniform_sampling(randPoints, P, rng, WalkType, walkL, numpoints, StartingPoint, nburns);
        } else {
            uniform_sampling<BilliardWalk>(randPoints, P, rng, walkL, numpoints,
                     StartingPoint, nburns);
        }
        break;
    case accelarated_billiard:
        if(set_L) {
            AcceleratedBilliardWalk WalkType(L);
            uniform_sampling(randPoints, P, rng, WalkType, walkL, numpoints, StartingPoint, nburns);
        } else {
            uniform_sampling<AcceleratedBilliardWalk>(randPoints, P, rng, walkL, numpoints,
                     StartingPoint, nburns);
        }
        break;
    case ball_walk:
        if (set_L) {
            if (gaussian) {
                GaussianBallWalk WalkType(L);
                gaussian_sampling(randPoints, P, rng, WalkType, walkL, numpoints, a,
                                       StartingPoint, nburns);
            } else {
                BallWalk WalkType(L);
                uniform_sampling(randPoints, P, rng, WalkType, walkL, numpoints,
                                       StartingPoint, nburns);
            }
        } else {
            if(gaussian) {
                gaussian_sampling<GaussianBallWalk>(randPoints, P, rng, walkL, numpoints,
                                                 a, StartingPoint, nburns);
            } else {
                uniform_sampling<BallWalk>(randPoints, P, rng, walkL, numpoints,
                                                 StartingPoint, nburns);
            }
        }
        break;
    default:
        throw Rcpp::exception("Unknown random walk!");
    }
}

//' Sample uniformly, normally distributed, or logconcave distributed points from a convex Polytope (H-polytope, V-polytope, zonotope or intersection of two V-polytopes).
//'
//' @param P A convex polytope. It is an object from class (a) Hpolytope or (b) Vpolytope or (c) Zonotope or (d) VpolytopeIntersection.
//' @param n The number of points that the function is going to sample from the convex polytope.
//' @param random_walk Optional. A list that declares the random walk and some related parameters as follows:
//' \itemize{
//' \item{\code{walk} }{ A string to declare the random walk: i) \code{'CDHR'} for Coordinate Directions Hit-and-Run, ii) \code{'RDHR'} for Random Directions Hit-and-Run, iii) \code{'BaW'} for Ball Walk, iv) \code{'BiW'} for Billiard walk, v) \code{'dikin'} for dikin walk, vi) \code{'vaidya'} for vaidya walk, vii) \code{'john'} for john walk, viii) \code{'BCDHR'} boundary sampling by keeping the extreme points of CDHR or ix) \code{'BRDHR'} boundary sampling by keeping the extreme points of RDHR x) \code{'HMC'} for Hamiltonian Monte Carlo (logconcave) xi) \code{'ULD'} for Underdamped Langevin Dynamics using the Randomized Midpoint Method. The default walk is \code{'aBiW'} for the uniform distribution or \code{'CDHR'} for the Gaussian distribution and H-polytopes and \code{'BiW'} or \code{'RDHR'} for the same distributions and V-polytopes and zonotopes.}
//' \item{\code{walk_length} }{ The number of the steps per generated point for the random walk. The default value is \eqn{1}.}
//' \item{\code{nburns} }{ The number of points to burn before start sampling. The default value is \eqn{1}.}
//' \item{\code{starting_point} }{ A \eqn{d}-dimensional numerical vector that declares a starting point in the interior of the polytope for the random walk. The default choice is the center of the ball as that one computed by the function \code{inner_ball()}.}
//' \item{\code{BaW_rad} }{ The radius for the ball walk.}
//' \item{\code{L} }{ The maximum length of the billiard trajectory or the radius for the step of dikin, vaidya or john walk.}
//' \item{\code{solver}} {Specify ODE solver for logconcave sampling. Options are i) leapfrog, ii) euler iii) runge-kutta iv) richardson}
//' \item{\code{step_size} {Optionally chosen step size for logconcave sampling. Defaults to a theoretical value if not provided.}}
//' }
//' @param distribution Optional. A list that declares the target density and some related parameters as follows:
//' \itemize{
//' \item{\code{density} }{ A string: (a) \code{'uniform'} for the uniform distribution or b) \code{'gaussian'} for the multidimensional spherical distribution. The default target distribution is uniform. c) Logconcave with form proportional to exp(-f(x)) where f(x) is L-smooth and m-strongly-convex. }
//' \item{\code{variance} }{ The variance of the multidimensional spherical gaussian. The default value is 1.}
//' \item{\code{mode} }{ A \eqn{d}-dimensional numerical vector that declares the mode of the Gaussian distribution. The default choice is the center of the as that one computed by the function \code{inner_ball()}.}
//' \item{\code{L_}} { Smoothness constant (for logconcave). }
//' \item{\code{m}} { Strong-convexity constant (for logconcave). }
//' \item{\code{negative_logprob}} { Negative log-probability (for logconcave). }
//' \item{\code{negative_logprob_gradient}} { Negative log-probability gradient (for logconcave). }
//' }
//' @param seed Optional. A fixed seed for the number generator.
//'
//' @references \cite{Robert L. Smith,
//' \dQuote{Efficient Monte Carlo Procedures for Generating Points Uniformly Distributed Over Bounded Regions,} \emph{Operations Research,} 1984.},
//'
//' @references \cite{B.T. Polyak, E.N. Gryazina,
//' \dQuote{Billiard walk - a new sampling algorithm for control and optimization,} \emph{IFAC Proceedings Volumes,} 2014.},
//'
//' @references \cite{Y. Chen, R. Dwivedi, M. J. Wainwright and B. Yu,
//' \dQuote{Fast MCMC Sampling Algorithms on Polytopes,} \emph{Journal of Machine Learning Research,} 2018.}
//'
//' @references \cite{Lee, Yin Tat, Ruoqi Shen, and Kevin Tian,
//' \dQuote{"Logsmooth Gradient Concentration and Tighter Runtimes for Metropolized Hamiltonian Monte Carlo,"} \emph{arXiv preprint arXiv:2002.04121}, 2020.}
//'
//' @references \cite{Shen, Ruoqi, and Yin Tat Lee,
//' \dQuote{"The randomized midpoint method for log-concave sampling.",} \emph{Advances in Neural Information Processing Systems}, 2019.}
//'
//' @return A \eqn{d\times n} matrix that contains, column-wise, the sampled points from the convex polytope P.
//' @examples
//' # uniform distribution from the 3d unit cube in H-representation using ball walk
//' P = gen_cube(3, 'H')
//' points = sample_points(P, n = 100, random_walk = list("walk" = "BaW", "walk_length" = 5))
//'
//' # gaussian distribution from the 2d unit simplex in H-representation with variance = 2
//' A = matrix(c(-1,0,0,-1,1,1), ncol=2, nrow=3, byrow=TRUE)
//' b = c(0,0,1)
//' P = Hpolytope$new(A,b)
//' points = sample_points(P, n = 100, distribution = list("density" = "gaussian", "variance" = 2))
//'
//' # uniform points from the boundary of a 2-dimensional random H-polytope
//' P = gen_rand_hpoly(2,20)
//' points = sample_points(P, n = 100, random_walk = list("walk" = "BRDHR"))
//'
//' # For sampling from logconcave densities see the examples directory
//'
//' @export
// [[Rcpp::export]]
Rcpp::NumericMatrix sample_points(Rcpp::Nullable<Rcpp::Reference> P,
                                  Rcpp::Nullable<unsigned int> n,
                                  Rcpp::Nullable<Rcpp::List> random_walk = R_NilValue,
                                  Rcpp::Nullable<Rcpp::List> distribution = R_NilValue,
                                  Rcpp::Nullable<double> seed = R_NilValue){

    typedef double NT;
    typedef Cartesian<NT>    Kernel;
    typedef BoostRandomNumberGenerator<boost::mt19937, NT> RNGType;
    typedef typename Kernel::Point    Point;
    typedef HPolytope <Point> Hpolytope;
    //typedef VPolytope<Point> Vpolytope;
    //typedef Zonotope <Point> zonotope;
    //typedef IntersectionOfVpoly<Vpolytope, RNGType> InterVP;
    typedef Eigen::Matrix<NT,Eigen::Dynamic,1> VT;
    typedef Eigen::Matrix<NT,Eigen::Dynamic,Eigen::Dynamic> MT;

    unsigned int type = Rcpp::as<Rcpp::Reference>(P).field("type"), dim = Rcpp::as<Rcpp::Reference>(P).field("dimension"),
          walkL = 1, numpoints, nburns = 0;

    //RcppFunctor::GradientFunctor<Point> *F = NULL;
    //RcppFunctor::FunctionFunctor<Point> *f = NULL;


    RNGType rng(dim);
    if (seed.isNotNull()) {
        unsigned seed_rcpp = Rcpp::as<double>(seed);
        rng.set_seed(seed_rcpp);
    }

    NT radius = 1.0, L;
    bool set_mode = false, gaussian = false, logconcave = false,
                    set_starting_point = false, set_L = false;

    random_walks walk;
    //ode_solvers solver; // Used only for logconcave sampling

    std::list<Point> randPoints;
    std::pair<Point, NT> InnerBall;
    Point mode(dim);

    numpoints = Rcpp::as<unsigned int>(n);
    if (numpoints <= 0) throw Rcpp::exception("The number of samples has to be a positive integer!");

    if (!distribution.isNotNull() || !Rcpp::as<Rcpp::List>(distribution).containsElementNamed("density")) {
        walk = billiard;
    } else if (
            Rcpp::as<std::string>(Rcpp::as<Rcpp::List>(distribution)["density"]).compare(std::string("uniform")) == 0) {
        walk = billiard;
    } else if (
            Rcpp::as<std::string>(Rcpp::as<Rcpp::List>(distribution)["density"]).compare(std::string("gaussian")) == 0) {
        gaussian = true;
    } else if (
            Rcpp::as<std::string>(Rcpp::as<Rcpp::List>(distribution)["density"]).compare(std::string("logconcave")) == 0) {
        logconcave = true;
    } else {
        throw Rcpp::exception("Wrong distribution!");
    }

    if (Rcpp::as<Rcpp::List>(distribution).containsElementNamed("mode")) {
        if (!(gaussian || logconcave)) throw Rcpp::exception("Mode is given only for Gaussian/logconcave sampling!");
        if (Rcpp::as<VT>(Rcpp::as<Rcpp::List>(distribution)["mode"]).size() != dim) {
            throw Rcpp::exception("Mode has to be a point in the same dimension as the polytope P");
        } else {
            set_mode = true;
            VT temp = Rcpp::as<VT>(Rcpp::as<Rcpp::List>(distribution)["mode"]);
            mode = Point(dim, std::vector<NT>(&temp[0], temp.data() + temp.cols() * temp.rows()));
        }
    }

    NT a = 0.5;
    if (Rcpp::as<Rcpp::List>(distribution).containsElementNamed("variance")) {
        a = 1.0 / (2.0 * Rcpp::as<NT>(Rcpp::as<Rcpp::List>(distribution)["variance"]));
        if (!gaussian) {
            Rcpp::warning("The variance can be set only for Gaussian sampling!");
        } else if (a <= 0.0) {
            throw Rcpp::exception("The variance has to be positive!");
        }
    }

    if (Rcpp::as<Rcpp::List>(distribution).containsElementNamed("negative_logprob") &&
        Rcpp::as<Rcpp::List>(distribution).containsElementNamed("negative_logprob_gradient")) {

        if (!logconcave) {
          throw Rcpp::exception("The negative logprob and its gradient can be set only for logconcave sampling!");
        }

        // Parse arguments
        Rcpp::Function negative_logprob = Rcpp::as<Rcpp::List>(distribution)["negative_logprob"];
        Rcpp::Function negative_logprob_gradient = Rcpp::as<Rcpp::List>(distribution)["negative_logprob_gradient"];

    


        

        // Create functors
        //RcppFunctor::parameters<NT> rcpp_functor_params(L_, m, eta, 2);
        //F = new RcppFunctor::GradientFunctor<Point>(rcpp_functor_params, negative_logprob_gradient);
        //f = new RcppFunctor::FunctionFunctor<Point>(rcpp_functor_params, negative_logprob);

    }


    if (!random_walk.isNotNull() || !Rcpp::as<Rcpp::List>(random_walk).containsElementNamed("walk")) {
        if (gaussian) {
            if (type == 1) {
                walk = cdhr;
            } else {
                walk = rdhr;
            }
        } else {
            if (type == 1) {
                walk = accelarated_billiard;
            } else {
                walk = billiard;
            }
        }
    } else if (Rcpp::as<std::string>(Rcpp::as<Rcpp::List>(random_walk)["walk"]).compare(std::string("CDHR")) == 0) {
        walk = cdhr;
    } else if (Rcpp::as<std::string>(Rcpp::as<Rcpp::List>(random_walk)["walk"]).compare(std::string("dikin")) == 0) {
        walk = dikin_walk;
        if (Rcpp::as<Rcpp::List>(random_walk).containsElementNamed("L")) {
            L = Rcpp::as<NT>(Rcpp::as<Rcpp::List>(random_walk)["L"]);
            set_L = true;
            if (L <= 0.0) throw Rcpp::exception("L must be a postitive number!");
        }
    } else if (Rcpp::as<std::string>(Rcpp::as<Rcpp::List>(random_walk)["walk"]).compare(std::string("vaidya")) == 0) {
        walk = vaidya_walk;
        if (Rcpp::as<Rcpp::List>(random_walk).containsElementNamed("L")) {
            L = Rcpp::as<NT>(Rcpp::as<Rcpp::List>(random_walk)["L"]);
            set_L = true;
            if (L <= 0.0) throw Rcpp::exception("L must be a postitive number!");
        }
    } else if (Rcpp::as<std::string>(Rcpp::as<Rcpp::List>(random_walk)["walk"]).compare(std::string("john")) == 0) {
        walk = john_walk;
        if (Rcpp::as<Rcpp::List>(random_walk).containsElementNamed("L")) {
            L = Rcpp::as<NT>(Rcpp::as<Rcpp::List>(random_walk)["L"]);
            set_L = true;
            if (L <= 0.0) throw Rcpp::exception("L must be a postitive number!");
        }
    } else if (Rcpp::as<std::string>(Rcpp::as<Rcpp::List>(random_walk)["walk"]).compare(std::string("RDHR")) == 0) {
        walk = rdhr;
    } else if (Rcpp::as<std::string>(Rcpp::as<Rcpp::List>(random_walk)["walk"]).compare(std::string("BaW")) == 0) {
        walk = ball_walk;
        if (Rcpp::as<Rcpp::List>(random_walk).containsElementNamed("BaW_rad")) {
            L = Rcpp::as<NT>(Rcpp::as<Rcpp::List>(random_walk)["BaW_rad"]);
            set_L = true;
            if (L <= 0.0) throw Rcpp::exception("BaW diameter must be a postitive number!");
        }
    } else if (Rcpp::as<std::string>(Rcpp::as<Rcpp::List>(random_walk)["walk"]).compare(std::string("BiW")) == 0) {
        if (gaussian) throw Rcpp::exception("Billiard walk can be used only for uniform sampling!");
        walk = billiard;
        if (Rcpp::as<Rcpp::List>(random_walk).containsElementNamed("L")) {
            L = Rcpp::as<NT>(Rcpp::as<Rcpp::List>(random_walk)["L"]);
            set_L = true;
            if (L <= 0.0) throw Rcpp::exception("L must be a postitive number!");
        }
    } else if (Rcpp::as<std::string>(Rcpp::as<Rcpp::List>(random_walk)["walk"]).compare(std::string("aBiW")) == 0) {
        if (gaussian) throw Rcpp::exception("Billiard walk can be used only for uniform sampling!");
        walk = accelarated_billiard;
        if (Rcpp::as<Rcpp::List>(random_walk).containsElementNamed("L")) {
            L = Rcpp::as<NT>(Rcpp::as<Rcpp::List>(random_walk)["L"]);
            set_L = true;
            if (L <= 0.0) throw Rcpp::exception("L must be a postitive number!");
        }
    } else if (Rcpp::as<std::string>(Rcpp::as<Rcpp::List>(random_walk)["walk"]).compare(std::string("BRDHR")) == 0) {
        if (gaussian) throw Rcpp::exception("Gaussian sampling from the boundary is not supported!");
        walk = brdhr;
    } else if (Rcpp::as<std::string>(Rcpp::as<Rcpp::List>(random_walk)["walk"]).compare(std::string("BCDHR")) == 0) {
        if (gaussian) throw Rcpp::exception("Gaussian sampling from the boundary is not supported!");
        walk = bcdhr;
    } else if (Rcpp::as<std::string>(Rcpp::as<Rcpp::List>(random_walk)["walk"]).compare(std::string("HMC")) == 0) {
        if (!logconcave) throw Rcpp::exception("HMC is not supported for non first-order sampling");
      walk = hmc;
    } else if (Rcpp::as<std::string>(Rcpp::as<Rcpp::List>(random_walk)["walk"]).compare(std::string("ULD")) == 0) {
      if (!logconcave) throw Rcpp::exception("ULD is not supported for non first-order sampling");
      walk = uld;
    } else {
        throw Rcpp::exception("Unknown walk type!");
    }

    Point StartingPoint;
    if (Rcpp::as<Rcpp::List>(random_walk).containsElementNamed("starting_point")) {
        if (Rcpp::as<VT>(Rcpp::as<Rcpp::List>(random_walk)["starting_point"]).size() != dim) {
            throw Rcpp::exception("Starting Point has to lie in the same dimension as the polytope P");
        } else {
            set_starting_point = true;
            VT temp = Rcpp::as<VT>(Rcpp::as<Rcpp::List>(random_walk)["starting_point"]);
            StartingPoint = Point(dim, std::vector<NT>(&temp[0], temp.data() + temp.cols() * temp.rows()));
        }
    }

    if (Rcpp::as<Rcpp::List>(random_walk).containsElementNamed("walk_length")) {
        walkL = Rcpp::as<int>(Rcpp::as<Rcpp::List>(random_walk)["walk_length"]);
        if (walkL <= 0) {
            throw Rcpp::exception("The walk length has to be a positive integer!");
        }
    }

    if (Rcpp::as<Rcpp::List>(random_walk).containsElementNamed("nburns")) {
        nburns = Rcpp::as<int>(Rcpp::as<Rcpp::List>(random_walk)["nburns"]);
        if (nburns < 0) {
            throw Rcpp::exception("The number of points to burn before sampling has to be a positive integer!");
        }
    }

    switch(type) {
        case 1: {
            // Hpolytope
            Hpolytope HP(dim, Rcpp::as<MT>(Rcpp::as<Rcpp::Reference>(P).field("A")),
                    Rcpp::as<VT>(Rcpp::as<Rcpp::Reference>(P).field("b")));
                    
            if (!set_starting_point || (!set_mode && gaussian) || !set_L) {
                InnerBall = HP.ComputeInnerBall();
                if (InnerBall.second < 0.0) throw Rcpp::exception("Unable to compute a feasible point.");
                if (!set_starting_point) StartingPoint = InnerBall.first;
                if (!set_mode && gaussian) mode = InnerBall.first;
            }
            HP.normalize();
            if (HP.is_in(StartingPoint) == 0) {
                throw Rcpp::exception("The given point is not in the interior of the polytope!");
            }
            if (gaussian) {
                StartingPoint = StartingPoint - mode;
                HP.shift(mode.getCoefficients());
            }
            sample_from_polytope(HP, type, rng, randPoints, walkL, numpoints, gaussian, a, L,
                                 StartingPoint, nburns, set_L, walk);
            break;
        }
        default:
        throw Rcpp::exception("Unknown convex body!");
    }

    if (numpoints % 2 == 1 && (walk == brdhr || walk == bcdhr)) numpoints--;
    MT RetMat(dim, numpoints);
    unsigned int jj = 0;

    for (typename std::list<Point>::iterator rpit = randPoints.begin(); rpit!=randPoints.end(); rpit++, jj++) {
        if (gaussian) {
            RetMat.col(jj) = (*rpit).getCoefficients() + mode.getCoefficients();
        } else {
            RetMat.col(jj) = (*rpit).getCoefficients();
        }
    }

    return Rcpp::wrap(RetMat);

}
