#define TVAI_UPSCALE_PARAMETER_MESSAGE "\n\n\
\tHyperion (hyp-1) parameters:\n\
\t\tsdr_ip: SDR highlight threshold (from 0.45 to 0.85)\n\
\t\thdr_ip_adjust: Amount of exposure adjustment to apply (from 0 to 1)\n\
\t\tsaturate: Amount of saturation adjustment to apply (from 0 to 1) \n\
\n\tSegment-Anything-2 (vsam) parameters:\
\n\t  clicks: [click expression]\
\n\t  Example: c0c1_2f0o0px1230y210x1260y390mx1218y561\
\n\t  - c controls which objects appear on which channels. Each 'c' is read in order, the first one representing channel 0 and the next channel 1 etc.\
\n\t    c0c1_2 puts object 0 in channel 0 and objects 1 and 2 in channel 1. \
\n\t  - f controls frame location for clicks. f0 followed by o and p/m x/y tokens marks the beginning of tracking, \
\n\t    whereas fXX without any o-specifiers signifies the end/break in tracking. \
\n\t  - o controls per-object clicks. o0px1230y210 means object 0 has plus (p) click (1230, 210). Any additional xy-pairs will be added to the same object.\
\n\t    mx1218y561 would mean a 'minus' point at (1218, 561). You may also use normalized coordinates. E.g. o1px.518y.279x.518y.396x0.518y0.755 \n \
"
#define TVAI_FRAME_INTERPOLATION_PARAMETER_MESSAGE "extra options"
#define TVAI_CAM_POSE_ESTIMATION_PARAMETER_MESSAGE "extra options"
#define TVAI_PARAMETER_ESTIMATION_PARAMETER_MESSAGE "extra options"
#define TVAI_STABILIZATION_PARAMETER_MESSAGE "extra options"
