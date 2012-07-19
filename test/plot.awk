#!/usr/bin/awk -f
BEGIN {
	count = 0
}
{
	if ($1 == "t")
		for (i = 0; $1 != "_"; i++) {
			line = getline;
			t[i] = $1;
			count++;
		}	
	else if ($1 == "y")
		for (i = 0; $1 != "_"; i++) {
			line = getline;
			y[i] = $1;
		}
	else if ($1 == "dy")
		for (i = 0; $1 != "_"; i++) {	
			line  = getline;
			dy[i] = $1;
		}
}
END {
	for (i = 0; i < count - 1; i++) {
		printf("%s %s\n", t[i], y[i]) > "exact.dat";	
		printf("%s %s\n", t[i], dy[i]) > "approx.dat";
	}
	system("gnuplot -p plot.dat");
}

