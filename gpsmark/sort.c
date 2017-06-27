#include "sort.h"

void merge(double *vec, size_t size) {
	int mid, i, j, k;
	double *tmp;
	tmp = (double*)malloc(size*sizeof(double));
	if(tmp == NULL) {
		return;
	}

	mid = size/2;

	i = 0;
	j = mid;
	k = 0;

	while(i < mid && j < size) {
		if(vec[i] <= vec[j]) {
			tmp[k] = vec[i];
			i++;
		} else {
			tmp[k] = vec[j];
			j++;
		}
		k++;
	}

	if(i == mid) {
		while(j < size){
			tmp[k] = vec[j];
			k++;
			j++;
		}
	} else {
		while(i < mid) {
			tmp[k] = vec[i];
			k++;
			i++;
		}
	}

	for(i = 0; i < size; i++) {
		vec[i] = tmp[i];
	}

	free(tmp);
}

void sort(double *vec, size_t size) {
	int mid;
	if(size > 1) {
		mid = size/2;
		sort(vec, mid);
		sort(vec+mid, size-mid);
		merge(vec, size);
	}
}
