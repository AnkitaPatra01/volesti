fast_preprocess_with_mosek <- function(P) {
  
  d = P$dimension
  Aeq = P$Aeq
  beq = P$beq
  
  A = P$A
  b = P$b
  
  m = dim(Aeq)[1]
  
  minWeights = c()
  maxWeights = c()
  
  row_ind = c()
  col_ind = c()
  values = c()
  
  for (i in 1:m) {
    inds = which(Aeq[i,] != 0)
    len_inds = length(inds)
    if (len_inds > 0){
      row_ind = c(row_ind, rep(i, len_inds))
      col_ind = c(col_ind, inds)
      values = c(values, Aeq[i, inds])
    }
  }
  
  prob <- list()
  prob$A <- Matrix::sparseMatrix(row_ind, col_ind, x=values)
  
  # Bound values for constraints
  prob$bc <- rbind(blc=beq, 
                   buc=beq)
  
  # Bound values for variables
  prob$bx <- rbind(blx=-b[(d+1):(2*d)], 
                   bux=b[1:d])
  
  for (j in 1:d) {
    
    prob$sense <- "min"
    prob$c <- A[j, ]
    r <- Rmosek::mosek(prob, list(verbose=0))
    stopifnot(identical(r$response$code, 0))
    min_dist = prob$c %*% r$sol$itr$xx
    #print(paste0("min_dist = ", min_dist))
    
    
    prob$sense <- "max"
    r <- Rmosek::mosek(prob, list(verbose=0))
    stopifnot(identical(r$response$code, 0))
    max_dist = prob$c %*% r$sol$itr$xx
    #print(paste0("mix_dist = ", max_dist))
    #print(" ")
    
    minWeights = c(minWeights, min_dist)
    maxWeights = c(maxWeights, max_dist)
    
    width = abs(max_dist - min_dist)
    
    if (width < 1e-07) {
      Aeq = rbind(Aeq, A[j,])
      beq = c(beq, min_dist)
    }
  }
  
  ret_list = list()
  ret_list$Aeq = Aeq
  ret_list$beq = beq
  ret_list$minWeights = minWeights
  ret_list$maxWeights = maxWeights
  ret_list$row_ind = row_ind
  ret_list$col_ind = col_ind
  ret_list$values = values
  
  return(ret_list)
}
