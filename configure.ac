AC_INIT(GtkGlk, 0.3, evin@users.sf.net)
AC_CONFIG_HEADERS(config.h)

AC_PROG_CC
AC_C_RESTRICT
AC_C_INLINE

# From glibc's configure.in:
    AC_CACHE_CHECK(whether __attribute__((visibility())) is supported,
                 libc_cv_visibility_attribute,
                 [cat > conftest.c <<EOF
                  int foo __attribute__ ((visibility ("hidden"))) = 1;
                  int bar __attribute__ ((visibility ("protected"))) = 1;
EOF
                  libc_cv_visibility_attribute=no
                  if ${CC} -Werror -S conftest.c -o conftest.s >/dev/null 2>&1; then
                    if grep '\.hidden.*foo' conftest.s >/dev/null; then
                      if grep '\.protected.*bar' conftest.s >/dev/null; then
                        libc_cv_visibility_attribute=yes
                      fi
                    fi
                  fi
                  rm -f conftest.[cs]
                 ])
    if test $libc_cv_visibility_attribute = yes; then
      AC_DEFINE(HAVE_VISIBILITY_ATTRIBUTE, [], [Compiler supports __attribute__ ((visibility()))])
    fi

AC_CHECK_LIB(SDL_mixer, Mix_OpenAudio,
             [ LIBS="$LIBS -lSDL_mixer"; AC_LIBOBJ([sound_sdl]) ],
             [ AC_LIBOBJ([sound_dummy]) ])

PKG_CHECK_MODULES(GTKGLK_GTK, gtk+-2.0 >= 2.1.0 gthread-2.0 >= 2.0)

# FIXME: Make gconf support optional
PKG_CHECK_MODULES(GTKGLK_GCONF, gconf-2.0 >= 2.0)


AC_OUTPUT(Makefile gtkglk.pc)
