#include <errno.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
/*
 * определяем константу THREAD_POOL_PARAM_T чтобы отключить
 предупреждения
 * компилятора при использовании функций семейства dispatch_*()
 */
#define THREAD_POOL_PARAM_T dispatch_context_t
#include <sys/iofunc.h>
#include <sys/dispatch.h>

static resmgr_connect_funcs_t connect_funcs;
static resmgr_io_funcs_t io_funcs;
static iofunc_attr_t attr;


main(int argc, char **argv)
{
	thread_pool_attr_t pool_attr;
	resmgr_attr_t resmgr_attr;
	dispatch_t *dpp;
	thread_pool_t *tpp;
	dispatch_context_t *ctp;
	int id;
	/* инициализация интерфейса диспетчеризации */
	if((dpp = dispatch_create()) == NULL) {
		fprintf(stderr,
				"%s: Unable to allocate dispatch handle.\n",
				argv[0]);
		return EXIT_FAILURE;
	}
	/* инициализация атрибутов АР - параметры IOV */
	memset(&resmgr_attr, 0, sizeof resmgr_attr);
	resmgr_attr.nparts_max = 1;
	resmgr_attr.msg_max_size = 2048;
	/* инициализация структуры функций-обработчиков сообщений */iofunc_func_init(_RESMGR_CONNECT_NFUNCS, &connect_funcs,
			_RESMGR_IO_NFUNCS, &io_funcs);
	/* инициализация атрибутов устройства*/
	iofunc_attr_init(&attr, S_IFNAM | 0666, 0, 0);
	/* прикрепление к точке монтирования в пространстве имён путей */
	id = resmgr_attach(
			dpp,
			/* хэндл интерфейса диспетчеризации */
			&resmgr_attr, /* атрибуты АР */
			"/dev/sample", /* точка монтирования */
			_FTYPE_ANY, /* open type */
			0, /* флаги */
			&connect_funcs, /* функции установления соединения */
			&io_funcs, /* функции ввода-вывода */
			&attr); /* хэндл атрибутов устройства */

	if(id == -1) {
		fprintf(stderr, "%s: Unable to attach name.\n", argv[0]);
		return EXIT_FAILURE;
	}
	/* инициализация атрибутов пула потоков */
	memset(&pool_attr, 0, sizeof pool_attr);
	pool_attr.handle = dpp;
	pool_attr.context_alloc = dispatch_context_alloc;
	pool_attr.block_func = dispatch_block;
	pool_attr.unblock_func = dispatch_unblock;
	pool_attr.handler_func = dispatch_handler;
	pool_attr.context_free = dispatch_context_free;pool_attr.lo_water = 2;
	pool_attr.hi_water = 4;
	pool_attr.increment = 1;
	pool_attr.maximum = 50;
	/* инициализация пула потоков */
	if((tpp = thread_pool_create(&pool_attr,
			POOL_FLAG_EXIT_SELF)) == NULL) {
		fprintf(stderr, "%s: Unable to initialize thread pool.\n",
				argv[0]);
		return EXIT_FAILURE;
	}
	/* запустить потоки, блокирующая функция */
	thread_pool_start(tpp);
	/* здесь вы не окажетесь, грустно */
}
