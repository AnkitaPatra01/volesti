central_scaling <- function(A, b) {
  
  m = dim(A)[1]
  d = dim(A)[2]
  
  z=get_max_inner_ball(A, b)
  T_scale = diag(d)
  scale_shift = rep(0, d)
  scale_applied = TRUE
  
  if (z$radius > 1e-05) {
    result_list = list()
    result_list$A = A
    result_list$b = b
    result_list$T_scale = T_scale
    result_list$scale_shift = scale_shift
    result_list$center = z$center
    result_list$radius = z$radius
    scale_applied = FALSE
    result_list$scale_applied = scale_applied
    
    return(result_list)
  }
  
  R =  z$radius
  CENTER = z$center
  reduce_factor = 1
  done = FALSE
  max_iter = ceiling(log10(1/(z$radius[1,1]))) - 1
  iter = 0
  
  while(!done & max_iter > 0) {
    iter = iter + 1
    print(paste0("iter = ",iter,", max_iter = ", max_iter))
    scale_shift = z$center
    brep = b - A %*% scale_shift
    scale_factor = 1/(z$radius[1,1]) 
    scale_factor = scale_factor / reduce_factor
    brep2 = brep * (scale_factor)
    a = try(get_max_inner_ball(A, brep2))
    if (class(a)=="list") {
      print(a$radius)
      print(scale_factor)
      if (a$radius > z$radius) {
        b = brep2
        T_scale = diag(d) * scale_factor
        R =  a$radius
        CENTER = a$center
        break
      } else {
        reduce_factor = reduce_factor * 10
      }
    } else {
      reduce_factor = reduce_factor * 10
    }
    if (iter == max_iter) {
      T_scale = diag(d)
      scale_shift = rep(0, d)
      scale_applied = FALSE
      print("we exceed maximmum number of iterations for scaling")
      break
    }
  }
  
  result_list = list()
  result_list$A = A
  result_list$b = b
  result_list$T_scale = T_scale
  result_list$scale_shift = scale_shift
  result_list$center = CENTER
  result_list$radius = R
  result_list$scale_applied = scale_applied
  
  return(result_list)
}
  
  