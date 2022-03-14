#!/usr/bin/perl

@filters = <*.c>;
for my $filter (@filters){
	($filterName, $extension) = split(/\./,$filter);
	printf("Building Filter: $filterName\n");
	system("./compile.sh $filterName");
}
