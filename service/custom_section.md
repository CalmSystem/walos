By default services functions signatures are exported as `__{module}_{name}_sign` globals
pointing to an array of `argc` w_fn_decl_val in data section.

Those globals cannot easly be automaticly strip if the function is unused.

As a fallback and optimisation a custom section named `w` can be used to put signatures.
```
signsec      ::= section_0(signdata)
signdata     ::= n:name                if (n == 'w')
                 signsubsec?
signsubsec   ::= vec(signdesc)
signdesc     ::= mod:name nm:name s:signval
signval      ::= vec(i8)
```
