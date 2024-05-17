extern void print(int);
extern int read();

int func(int i){
	int a;
	int	b;
	b = i+10;	

	while (a < i){
		if (b > 0) 
			a = a + b;
		else 
			a = a - b;
		
	}

	return b;

}
