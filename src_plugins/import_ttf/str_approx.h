extern char *str_approx_comment;

int stroke_approx_conic_to(const FT_Vector *control, const FT_Vector *to, void *s_);
int stroke_approx_cubic_to(const FT_Vector *control1, const FT_Vector *control2, const FT_Vector *to, void *s_);
