%%
\*SEGMENT { printf("segment\n"); }
BEGIN { printf("begin\n"); }
BEGIN { printf("begin\n"); }

\n { printf("NL\n");}
. ;
%%

main()
{
    yylex();
}