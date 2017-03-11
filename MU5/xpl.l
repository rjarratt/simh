%%
SEGMENT { printf("segment\n"); }
\n { printf("NL\n");}
. ;
%%

main()
{
    yylex();
}