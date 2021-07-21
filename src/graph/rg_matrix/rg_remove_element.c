/*
* Copyright 2018-2021 Redis Labs Ltd. and Contributors
*
* This file is available under the Redis Labs Source Available License Agreement
*/

#include "RG.h"
#include "rg_matrix.h"
#include "rg_utils.h"
#include "../../util/arr.h"
#include "../../util/rmalloc.h"

GrB_Info RG_Matrix_removeElement
(
    RG_Matrix C,                    // matrix to remove entry from
    GrB_Index i,                    // row index
    GrB_Index j                     // column index
) {
	ASSERT(C);
	RG_Matrix_checkBounds(C, i, j);

	uint64_t    m_x;
	uint64_t    dm_x;
	uint64_t    dp_x;
	GrB_Info    info;
	bool        in_m        =  false;
	bool        in_dp       =  false;
	bool        in_dm       =  false;
	bool        multi_edge  =  RG_Matrix_getMultiEdge(C);
	GrB_Matrix  m           =  RG_MATRIX_M(C);
	GrB_Matrix  dp          =  RG_MATRIX_DELTA_PLUS(C);
	GrB_Matrix  dm          =  RG_MATRIX_DELTA_MINUS(C);

	if(C->maintain_transpose) {
		info = RG_Matrix_removeElement(C->transposed, j, i);
		if(info != GrB_SUCCESS) {
			return info;
		} 
	}

	info = GrB_Matrix_extractElement(&m_x, m, i, j);
	in_m = (info == GrB_SUCCESS);

	info = GrB_Matrix_extractElement(&dp_x, dp, i, j);
	in_dp = (info == GrB_SUCCESS);

	info = GrB_Matrix_extractElement(&dm_x, dm, i, j);
	in_dm = (info == GrB_SUCCESS);

	// mask 'in_m' incase it is marked for deletion
	in_m = in_m && !(in_dm);

	// entry missing from both 'm' and 'dp'
	if(!(in_m || in_dp)) {
		return GrB_NO_VALUE;
	}

	// entry can't exists in both 'm' and 'dp'
	ASSERT(in_m != in_dp);

	//--------------------------------------------------------------------------
	// entry exists in 'M'
	//--------------------------------------------------------------------------

	if(in_m) {
		if(multi_edge) {
			// free multi-edge entry, leave M[i,j] dirty
			if((SINGLE_EDGE(m_x)) == false) {
				m_x = CLEAR_MSB(m_x);
				array_free(m_x);
			}
		}

		// mark deletion in delta minus
		info = GrB_Matrix_setElement(dm, true, i, j);
		ASSERT(info == GrB_SUCCESS);
	}

	//--------------------------------------------------------------------------
	// entry exists in 'delta-plus'
	//--------------------------------------------------------------------------

	if(in_dp) {
		// remove entry from 'dp'
		if(multi_edge) {
			// free multi-edge entry
			if((SINGLE_EDGE(dp_x)) == false) {
				dp_x = CLEAR_MSB(dp_x);
				array_free(dp_x);
			}
		}

		info = GrB_Matrix_removeElement(dp, i, j);
		ASSERT(info == GrB_SUCCESS);
	}

#ifdef RG_DEBUG
	RG_Matrix_validateState(C, i, j);
#endif

	RG_Matrix_setDirty(C);
	return info;
}

