
#ifndef TOPAZ_RLM_EXPORT_H
#define TOPAZ_RLM_EXPORT_H

#ifdef TOPAZ_RLM_STATIC_DEFINE
#  define TOPAZ_RLM_EXPORT
#  define TOPAZ_RLM_NO_EXPORT
#else
#  ifndef TOPAZ_RLM_EXPORT
#    ifdef topaz_rlm_EXPORTS
        /* We are building this library */
#      define TOPAZ_RLM_EXPORT __attribute__((visibility("default")))
#    else
        /* We are using this library */
#      define TOPAZ_RLM_EXPORT __attribute__((visibility("default")))
#    endif
#  endif

#  ifndef TOPAZ_RLM_NO_EXPORT
#    define TOPAZ_RLM_NO_EXPORT __attribute__((visibility("hidden")))
#  endif
#endif

#ifndef TOPAZ_RLM_DEPRECATED
#  define TOPAZ_RLM_DEPRECATED __attribute__ ((__deprecated__))
#endif

#ifndef TOPAZ_RLM_DEPRECATED_EXPORT
#  define TOPAZ_RLM_DEPRECATED_EXPORT TOPAZ_RLM_EXPORT TOPAZ_RLM_DEPRECATED
#endif

#ifndef TOPAZ_RLM_DEPRECATED_NO_EXPORT
#  define TOPAZ_RLM_DEPRECATED_NO_EXPORT TOPAZ_RLM_NO_EXPORT TOPAZ_RLM_DEPRECATED
#endif

#if 0 /* DEFINE_NO_DEPRECATED */
#  ifndef TOPAZ_RLM_NO_DEPRECATED
#    define TOPAZ_RLM_NO_DEPRECATED
#  endif
#endif

#endif /* TOPAZ_RLM_EXPORT_H */
