#include "Epetra_Map.h"
#include "Epetra_LocalMap.h"
#include "Epetra_FECrsMatrix.h"
#include "../epetra_test_err.h"

int MatrixTests(const Epetra_Map & map, const Epetra_LocalMap & LocalMap, int NumVectors,
		    bool verbose);

int MultiVectorTests(const Epetra_Map & Map, int NumVectors, bool verbose);

