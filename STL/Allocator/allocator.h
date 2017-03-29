/*
*�ļ�˵��:ʵ��һ���򵥵Ŀռ�������
*����:��С��
*����:2017-03-29
*���ɿ�������:Microsoft Visual Studio 2010 
*/
#pragma once

//////////////һ���ռ�������//////////////
typedef void(*ALLOC_OOM_FUN)();	//һ������ָ���typedef

template<int inst>
class _MallocAllocTemplate{
public:
	//�����ڴ�,�ȵ���malloc,�������ʧ��,�ٵ���OomMalloc����
	static void *Allocate(size_t n){
		void *ret = malloc(n);
		if(NULL == ret){
			ret = _OomMalloc(n);
		}
		return ret;
	}

	//׷���ڴ�,ͬ��
	static void *Reallocate(void *p,size_t n){
		void *ret = realloc(p,n);
		if(NULL==ret){
			ret = _OomRealloc(p,n):
		}
		return ret;
	}

	//�ͷ��ڴ�,ֱ�ӵ���free����
	static void Deallocate(void *p){
		free(p);
	}

	//�ͷ��ڴ�,ֱ�ӵ���free����
	static void Deallocate(void *p,size_t n){
		free(p);
	}

	//���þ������,������ԭ�еľ������
	static ALLOC_OOM_FUN SetMallocHandler(ALLOC_OOM_FUN f){
		ALLOC_OOM_FUN old = _sMallocAllocOomHandler;
		_sMallocAllocOomHandler = f;
		return old;
	}
private:
	//�����ڴ�(mallocʧ��ʱ���ñ�����,����handler�ͷ��ڴ��,����ͨ��malloc�����ڴ�)
	static void *_OomMalloc(size_t n){
		ALLOC_OOM_FUN handler;
		void *ret;
		for(;;){

			//����������
			handler = _sMallocAllocOomHandler;
			if(NULL == handler){
				std::cerr<<"�ڴ治��"<<std::endl;
				exit(-1);
			}

			handler();	//ִ�о������(��ž����ͷ�һ���������õ��ڴ�)
			
			ret = malloc(n);
			if(ret){
				return ret;
			}
		}
	}

	//ͬ��
	static void *_OomRealloc(void *p,size_t n){
		ALLOC_OOM_FUN handler;
		void *ret;
		for(;;;){
			handler = _sMallocAllocOomHandler;
			if(NULL == handler){
				std::cerr<<"�ڴ治��"<<std::endl;
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
	static ALLOC_OOM_FUN _sMallocAllocOomHandler;	//�������
};
template<int inst>
ALLOC_OOM_FUN _MallocAllocTemplate<inst>::_sMallocAllocOomHandler = 0;



////////////////�����ռ�������//////////////////
template<bool threads,int inst>
class __DefaultAllocTempate{
public:
	static void *Allocate(size_t n){
		
		//Ҫ������ڴ����__MAX_BYTES(128),����һ���ռ�������
		if(n > __MAX_BYTES){
			return _MallocAllocTemplate<0>::Allocate(n);
		}

		//�������������±�
		size_t index = FREELIST_INDEX(n);
		void *ret = NULL;

		Obj *head = _freeList[index];
		if(head==NULL){
			//��������û�нڵ�,����Refill����,���ڴ�����п�ҵ���������
			return Refill(n);
		}
		//������������ֵ,����ͷɾ
		_freeList[index] = head->_freeListLink;
		return head;			
	}

	static void Deallocate(void *p,size_t n){
		
		//n����128,����һ���ռ�����������
		if(n > __MAX_BYTES){
			_MallocAllocTemplate<0>::Deallocate(p,n);
		}else{
			//PS:���̻߳������Ǽ���
			size_t index = FREELIST_INDEX(n);
			//ͷ�����������
			Obj *tmp = (Obj*)p;
			tmp->_freeListLink = _freeList[index];
			_freeList[index] = tmp;
		}
	}

private:
	//1-7��9-15��17-23��25-31���뵽8��16��24��32....
	static size_t ROUND_UP(size_t bytes){
		return (bytes+__ALIGN-1)& ~(__ALIGN-1);
	}

	//��ȡ����������±�
	static size_t FREELIST_INDEX(size_t bytes){
		return (bytes+__ALIGN-1)/__ALIGN - 1;
	}

	//��ȡ����ڴ沢��������������
	static void *Refill(size_t n){
		//ÿ�δ��ڴ��ȡ20�������С���ڴ�
		int nobjs = 20;
		char *chunk = ChunkAlloc(n,nobjs);
		
		//�ڴ����ֻ�ܷ��䵽һ������Ĵ�С,��ֱ�ӷ���
		if(nobjs == 1){
			return chunk;
		}

		//���ٻ�ȡ����������,����һ���ڴ����Ϊ�������
		Obj *result,*cur;
		size_t index = FREELIST_INDEX(n);
		result = (Obj*)chunk;

		//��ʣ�µ��ڴ�������������
		cur = (Obj*)(chunk+n);
		_freeList[index] = cur;		//��index���λ��,��Ȼû���ڴ��,��˰ѵڶ����ڴ���index

		//��һ�������������,�ڶ����index,ʣ��nobjs�����index����
		for(int i=2; i<nobjs; ++i){
			cur->_freeListLink = (Obj*)(chunk+n*i);
			cur = cur->_freeListLink;
		}
		cur->_freeListLink = NULL;

		return result;	
	}

	//���ڴ���з������ڴ�
	static char *ChunkAlloc(size_t size,int &nobjs){
		char* result = NULL;
		size_t bytesNeed = size*nobjs;
		size_t bytesLeft = _endFree-_startFree;
		
		//�ڴ�ؿռ��ԣ
		if(bytesLeft>=bytesNeed){
			result = _startFree;
			_startFree += bytesNeed;
		//�ڴ��ֻ�ܷ���һ�����ڴ�
		}else if(bytesLeft>=size){
			nobjs = bytesLeft/size;		//�����ܷ��伸���ڴ��
			result = _startFree;	
			_startFree += nobjs*size;
		//�ڴ��һ�����󶼿�������
		}else{
			//�ڴ�ػ���ʣ���ڴ�
			if(bytesLeft > 0){
				//��ǰʣ���ڴ������������е��±�
				size_t index = FREELIST_INDEX(bytesLeft);
				//��ʣ���ڴ�ͷ��������������
				((Obj*)_startFree)->_freeListLink = _freeList[index];
				_freeList[index] = (Obj*)_startFree;
				//�ڴ���ѿ�
				_startFree = NULL;
			}

			//��ϵͳ���з����ڴ�
			size_t bytesToGet = 2*bytesNeed + ROUND_UP(_heapSize>>4);
			_startFree = (char*) malloc(bytesToGet);

			//���ϵͳ���з��䲻����ô���ڴ�
			if(_startFree==NULL){
				//�����������и���Ľڵ��з���
				for(int i=size; i<=__MAX_BYTES; i+=__ALIGN){
					Obj* head= _freeList[FREELIST_INDEX(i)];
					if(head){
						//����������Ľڵ���ڴ��
						_startFree = (char*)head;
						//ͷɾ�ڵ�
						_freeList[FREELIST_INDEX(i)] = head->_freeListLink;
						_endFree = _startFree + i;
						return ChunkAlloc(size,nobjs);
					}	
				}
				_endFree = 0;
				
				//���һ����������,�ٴε���һ���ռ�������,�ڴ���������а취�ͷ��ڴ�
				_startFree = (char*)_MallocAllocTemplate<0>::Allocate(bytesToGet);

			}
			_heapSize += bytesToGet;
			_endFree = _startFree + bytesToGet;
			return ChunkAlloc(size,nobjs);
		}
		return result;
	}

public:
		//��������ڵ㶨��
	union Obj{
		union Obj * _freeListLink;		//ָ����һ����������Ľڵ�
		char _clientData[1];			//��֪����ɶ��,�ٷ�ע��the client seen this
	};
private:
	enum{__ALIGN = 8};							//���м��
	enum{__MAX_BYTES = 128};					//���ֵ
	enum{__NFREELISTS = __MAX_BYTES/__ALIGN};	//��������Ĵ�С

	static Obj * volatile _freeList[__NFREELISTS];	//��������
	static char *_startFree;						//�ڴ����ʼ
	static char *_endFree;							//�ڴ�ؽ���
	static size_t _heapSize;						//��ϵͳ���з�����ܹ���С

};


template<bool threads,int inst>
char *__DefaultAllocTempate<threads,inst>::_startFree = 0;
template<bool threads,int inst>
char *__DefaultAllocTempate<threads,inst>::_endFree = 0;
template<bool threads,int inst>
size_t __DefaultAllocTempate<threads,inst>::_heapSize = 0;
template<bool threads,int inst>
typename __DefaultAllocTempate<threads,inst>::Obj * volatile __DefaultAllocTempate<threads,inst>:: _freeList[__NFREELISTS]={0};