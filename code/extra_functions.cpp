int  concatenate( char str1[] , char str2[] )
{
	int i=0;
	int count1=0 , count2=0;
	while(str1[i]!='\0')
	{
		i++;
	}
	count1+=i;
	i=0;
	while(str2[i]!='\0')
	{
		i++;
	}
	count2+=i;


	char res[count1+count2+1];
	for(int i=0;i<count1;i++)
	{
		res[i]=str1[i];
	}
	for(int i=0;i<=count2;i++)
	{
		res[i+count1]=str2[i];
	}
	return count1;
}