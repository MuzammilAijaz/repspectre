#ifndef QS_PORT_H_
#define QS_PORT_H_

#define QS_CTR_SIZE     4U
#define QS_TIME_SIZE    4U
#define QS_OBJ_PTR_SIZE 4U
#define QS_FUN_PTR_SIZE 4U

void QS_output(void);
void QS_rx_input(void);

#ifndef QP_PORT_H_
#include "qp_port.h"
#endif

#include "qs.h"

#endif
