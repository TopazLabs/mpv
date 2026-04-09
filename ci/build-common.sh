common_args="--werror \
-Dgpl=false \
-Dlibmpv=true \
-Dtests=true \
"

export CFLAGS="$CFLAGS -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=3"
