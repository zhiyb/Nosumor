SECTIONS
{
  list_module :
  {
    . = ALIGN(4);
    KEEP(*(list_module));
  } >TEXT AT >LOAD

  list_keyboard :
  {
    . = ALIGN(4);
    KEEP(*(list_keyboard));
  } >TEXT AT >LOAD

  list_systick :
  {
    . = ALIGN(4);
    KEEP(*(list_systick));
  } >TEXT AT >LOAD
}
