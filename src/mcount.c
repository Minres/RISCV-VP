/* The canonical name for the function is `_mcount' in both C and asm,
   but some old asm code might assume it's `mcount'.  */
void _mcount (void);
weak_alias (_mcount, mcount);

void _mcount (void)
{
  mcount_internal ((u_long) __builtin_return_address (1), (u_long) __builtin_return_address(0));
}
