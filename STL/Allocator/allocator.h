/*
*文件说明:实现一个简单的空间配置器
*作者:高小调
*日期:2017-03-29
*集成开发环境:Microsoft Visual Studio 2010 
*/
#pragma once

//////////////一级空间配置器//////////////
typedef void(*ALLOC_OOM_FUN)();	//一个函数指针的typedef

template<int inst>
class _MallocAllocTemplate{
public:
	//申请内存,先调用malloc,如果申请失败,再调用OomMalloc函数
	static void *Allocate(size_t n){
		void *ret = malloc(n);
		if(NULL == ret){
			ret = _OomMalloc(n);
		}
		return ret;
	}

	//追加内存,同上
	static void *Reallocate(void *p,size_t n){
		void *ret = realloc(p,n);
		if(NULL==ret){
			ret = _OomRealloc(p,n):
		}
		return ret;
	}

	//释放内存,直接调用free函数
	static void Deallocate(void *p){
		free(p);
	}

	//释放内存,直接调用free函数
	static void Deallocate(void *p,size_t n){
		free(p);
	}

	//设置句柄函数,并返回原有的句柄函数
	static ALLOC_OOM_FUN SetMallocHandler(ALLOC_OOM_FUN f){
		ALLOC_OOM_FUN old = _sMallocAllocOomHandler;
		_sMallocAllocOomHandler = f;
		return old;
	}
private:
	//申请内存(malloc失败时调用本函数,调用handler释放内存后,继续通过malloc申请内存)
	static void *_OomMalloc(size_t n){
		ALLOC_OOM_FUN handler;
		void *ret;
		for(;;){

			//传入句柄函数
			handler = _sMallocAllocOomHandler;
			if(NULL == handler){
				std::cerr<<"内存不足"<<std::endl;
				exit(-1);
			}

			handler();	//执行句柄函数(大概就是释放一下其他无用的内存)
			
			ret = malloc(n);
			if(ret){
				return ret;
			}
		}
	}

	//同上
	static void *_OomRealloc(void *p,size_t n){
		ALLOC_OOM_FUN handler;
		void *ret;
		for(;;;){
			handler = _sMallocAllocOomHandler;
			if(NULL == handler){
				std::cerr<<"内存不足"<<std::endl;
				exit(-1);
			}

			handler();

			ret = realloc(p,n);
			if(ret){
				return ret;
			}
		}
	}
private:
	static ALLOC_OOM_FUN _sMallocAllocOomHandler;	//句柄函数
};
template<int inst>
ALLOC_OOM_FUN _MallocAllocTemplate<inst>::_sMallocAllocOomHandler = 0;



////////////////二级空间配置器//////////////////
template<bool threads,int inst>
class __DefaultAllocTempate{
public:
	static void *Allocate(size_t n){
		
		//要申请的内存大于__MAX_BYTES(128),调用一级空间配置器
		if(n > __MAX_BYTES){
			return _MallocAllocTemplate<0>::Allocate(n);
		}

		//计算自由链表下标
		size_t index = FREELIST_INDEX(n);
		void *ret = NULL;

		Obj *head = _freeList[index];
		if(head==NULL){
			//自由链表没有节点,调用Refill函数,从内存池中切块挂到自由链表
			return Refill(n);
		}
		//自由链表里有值,进行头删
		_freeList[index] = head->_freeListLink;
		return head;			
	}

	static void Deallocate(void *p,size_t n){
		
		//n大于128,交给一级空间配置器处理
		if(n > __MAX_BYTES){
			_MallocAllocTemplate<0>::Deallocate(p,n);
		}else{
			//PS:多线程环境考虑加锁
			size_t index = FREELIST_INDEX(n);
			//头插回自由链表
			Obj *tmp = (Obj*)p;
			tmp->_freeListLink = _freeList[index];
			_freeList[index] = tmp;
		}
	}

private:
	//1-7、9-15、17-23、25-31对齐到8、16、24、32....
	static size_t ROUND_UP(size_t bytes){
		return (bytes+__ALIGN-1)& ~(__ALIGN-1);
	}

	//获取自由链表的下标
	static size_t FREELIST_INDEX(size_t bytes){
		return (bytes+__ALIGN-1)/__ALIGN - 1;
	}

	//获取大块内存并放入自由链表中
	static void *Refill(size_t n){
		//每次从内存池取20个对象大小的内存
		int nobjs = 20;
		char *chunk = ChunkAlloc(n,nobjs);
		
		//内存池中只能分配到一个对象的大小,则直接返回
		if(nobjs == 1){
			return chunk;
		}

		//至少获取了两个对象,将第一个内存块作为结果返回
		Obj *result,*cur;
		size_t index = FREELIST_INDEX(n);
		result = (Obj*)chunk;

		//把剩下的内存块挂入自由链表
		cur = (Obj*)(chunk+n);
		_freeList[index] = cur;		//在index这个位置,必然没有内存块,因此把第二个内存块给index

		//第一块用作结果返回,第二块给index,剩下nobjs块挂在index后面
		for(int i=2; i<nobjs; ++i){
			cur->_freeListLink = (Obj*)(chunk+n*i);
			cur = cur->_freeListLink;
		}
		cur->_freeListLink = NULL;

		return result;	
	}

	//从内存池中分配大块内存
	static char *ChunkAlloc(size_t size,int &nobjs){
		char* result = NULL;
		size_t bytesNeed = size*nobjs;
		size_t bytesLeft = _endFree-_startFree;
		
		//内存池空间充裕
		if(bytesLeft>=bytesNeed){
			result = _startFree;
			_startFree += bytesNeed;
		//内存池只能分配一部分内存
		}else if(bytesLeft>=size){
			nobjs = bytesLeft/size;		//计算能分配几个内存块
			result = _startFree;	
			_startFree += nobjs*size;
		//内存池一个对象都开不出来
		}else{
			//内存池还有剩余内存
			if(bytesLeft > 0){
				//当前剩余内存在自由链表中的下标
				size_t index = FREELIST_INDEX(bytesLeft);
				//将剩余内存头插在自由链表中
				((Obj*)_startFree)->_freeListLink = _freeList[index];
				_freeList[index] = (Obj*)_startFree;
				//内存池已空
				_startFree = NULL;
			}

			//从系统堆中分配内存
			size_t bytesToGet = 2*bytesNeed + ROUND_UP(_heapSize>>4);
			_startFree = (char*) malloc(bytesToGet);

			//如果系统堆中分配不出这么多内存
			if(_startFree==NULL){
				//在自由链表中更大的节点中分配
				for(int i=size; i<=__MAX_BYTES; i+=__ALIGN){
					Obj* head= _freeList[FREELIST_INDEX(i)];
					if(head){
						//把自由链表的节点给内存池
						_startFree = (char*)head;
						//头删节点
						_freeList[FREELIST_INDEX(i)] = head->_freeListLink;
						_endFree = _startFree + i;
						return ChunkAlloc(size,nobjs);
					}	
				}
				_endFree = 0;
				
				//最后一根救命稻草,再次调用一级空间配置器,期待句柄函数有办法释放内存
				_startFree = (char*)_MallocAllocTemplate<0>::Allocate(bytesToGet);

			}
			_heapSize += bytesToGet;
			_endFree = _startFree + bytesToGet;
			return ChunkAlloc(size,nobjs);
		}
		return result;
	}

public:
		//自由链表节点定义
	union Obj{
		union Obj * _freeListLink;		//指向下一个自由链表的节点
		char _clientData[1];			//不知道有啥用,官方注释the client seen this
	};
private:
	enum{__ALIGN = 8};							//排列间隔
	enum{__MAX_BYTES = 128};					//最大值
	enum{__NFREELISTS = __MAX_BYTES/__ALIGN};	//自由链表的大小

	static Obj * volatile _freeList[__NFREELISTS];	//自由链表
	static char *_startFree;						//内存池起始
	static char *_endFree;							//内存池结束
	static size_t _heapSize;						//从系统堆中分配的总共大小

};


template<bool threads,int inst>
char *__DefaultAllocTempate<threads,inst>::_startFree = 0;
template<bool threads,int inst>
char *__DefaultAllocTempate<threads,inst>::_endFree = 0;
template<bool threads,int inst>
size_t __DefaultAllocTempate<threads,inst>::_heapSize = 0;
template<bool threads,int inst>
typename __DefaultAllocTempate<threads,inst>::Obj * volatile __DefaultAllocTempate<threads,inst>:: _freeList[__NFREELISTS]={0};