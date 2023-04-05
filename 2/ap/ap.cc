
#include <bbs.h>
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/dispatch.h>
#include <sys/iofunc.h>
#include <unistd.h>

static resmgr_connect_funcs_t connect_funcs;
static resmgr_io_funcs_t io_funcs;
static iofunc_attr_t attr;

bbs::BBSParams params;

uint32_t last_int;

bool parity(uint32_t n) {
  int count = 0;
  while (n) {
    count += n & 1;
    n >>= 1;
  }

  return (count % 2 != 0);
}

uint32_t bbs_algo(uint32_t x0, uint32_t p, uint32_t q) {
  // Ну тот самый алгоритм
  uint32_t x1 = (x0 * x0) % (p * q);
  return x1;
}

uint32_t psp() {
  // ПСП
  uint32_t packed = 0;
  for (size_t i = 0; i < 32; i++) {
    last_int = bbs_algo(last_int, params.p, params.q);
    packed |= (parity(last_int) & 1) << i;
  }
  return packed;
}

int io_devctl(resmgr_context_t *ctp, io_devctl_t *msg, void *extra) {
  iofunc_ocb_t *ocb = reinterpret_cast<iofunc_ocb_t *>(extra);
  union {
    bbs::BBSParams data;
    uint32_t data32;
    /* ... other devctl types you can receive */
  } *rx_data;
  int sts;
  // 1) see if it's a standard devctl()
  if ((sts = iofunc_devctl_default(ctp, msg, ocb)) != _RESMGR_DEFAULT) {
    return (sts);
  }

  // rx_data = _DEVCTL_DATA(msg->i);

  bbs::BBSParams *rx_params_data;
  uint32_t *return_number;
  uint32_t nbytes = 0;
  uint32_t result_number;
  // 2) see which command it was, and act on it
  switch (msg->i.dcmd) {
  case MY_DEVCTL_SET_PARAM:
    rx_params_data = (bbs::BBSParams *)_DEVCTL_DATA(msg->i);
    params.p = rx_params_data->p;
    params.q = rx_params_data->q;
    params.seed = rx_params_data->seed;
    last_int = rx_params_data->seed;
    printf("\nSET PARAM!!\np=%d,q=%d,seed=%d\n", params.p, params.q,
           params.seed);
    break;
  case MY_DEVCTL_START:

    return_number = (uint32_t *)_DEVCTL_DATA(msg->i);
    result_number = psp();
    *return_number = result_number;
    memset(&msg->o, 0, sizeof(msg->o));
    /*
    If you wanted to pass something different to the return
    field of the devctl() you could do it through this member.
    See note 5.
    */
    msg->o.ret_val = result_number;
    nbytes = sizeof(result_number);
    /* Indicate the number of bytes and return the message */
    msg->o.nbytes = nbytes;
    return (_RESMGR_PTR(ctp, &msg->o, sizeof(msg->o) + nbytes));

    printf("\nSTART\n");
    break;

  default:
    printf("\nGOT MESSAGE!!\n");
    return (ENOSYS);
  }
  // 4) tell the client it worked
  memset(&msg->o, 0, sizeof(msg->o));
  SETIOV(ctp->iov, &msg->o, sizeof(msg->o));
  return (_RESMGR_NPARTS(1));
}

int main(int argc, char **argv) {
  /* declare variables we'll be using */
  resmgr_attr_t resmgr_attr;
  dispatch_t *dpp;
  dispatch_context_t *ctp;
  int id;

  /* initialize dispatch interface */
  if ((dpp = dispatch_create()) == NULL) {
    fprintf(stderr, "%s: Unable to allocate dispatch handle.\n", argv[0]);
    return EXIT_FAILURE;
  }

  /* initialize resource manager attributes */
  memset(&resmgr_attr, 0, sizeof resmgr_attr);
  resmgr_attr.nparts_max = 1;
  resmgr_attr.msg_max_size = 2048;

  /* initialize functions for handling messages */
  iofunc_func_init(_RESMGR_CONNECT_NFUNCS, &connect_funcs, _RESMGR_IO_NFUNCS,
                   &io_funcs);

  /* initialize attribute structure used by the device */
  iofunc_attr_init(&attr, S_IFNAM | 0666, 0, 0);

  /* attach our device name */
  id = resmgr_attach(dpp,              /* dispatch handle        */
                     &resmgr_attr,     /* resource manager attrs */
                     "/dev/cryptobbs", /* device name            */
                     _FTYPE_ANY,       /* open type              */
                     0,                /* flags                  */
                     &connect_funcs,   /* connect routines       */
                     &io_funcs,        /* I/O routines           */
                     &attr);           /* handle                 */
  if (id == -1) {
    fprintf(stderr, "%s: Unable to attach name.\n", argv[0]);
    return EXIT_FAILURE;
  }

  io_funcs.devctl = io_devctl;

  /* allocate a context structure */
  ctp = dispatch_context_alloc(dpp);

  /* start the resource manager message loop */
  while (1) {
    if ((ctp = dispatch_block(ctp)) == NULL) {
      fprintf(stderr, "block error\n");
      return EXIT_FAILURE;
    }
    dispatch_handler(ctp);
  }
  return EXIT_SUCCESS; // never go here
}

// Description	Resource	Path	Location	Type
// invalid conversion from 'int (*)(resmgr_context_t*, io_devctl_t*, iofunc_ocb_t*) {aka int (*)(_resmgr_context*, io_devctl_t*, _iofunc_ocb*)}' to 'int (*)(resmgr_context_t*, io_devctl_t*, void*) {aka int (*)(_resmgr_context*, io_devctl_t*, void*)}' [-fpermissive]	ap.cc	/ap	line 144	C/C++ Problem
