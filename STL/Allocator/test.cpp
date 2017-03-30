/*
*�ļ�˵��:�����ļ�
*����:��С��
*����:2017-03-26
*���ɿ�������:Microsoft Visual Studio 2010 
*/
#include<iostream>
#include<cstdio>
#include<malloc.h>
using namespace std;
#include"allocator.h"

//�Կռ�����������һ���װ
template<typename T,typename Alloc>
class SimpleAlloc{
public:
	//������new
	static T* Alloc(){
		//���ٿռ�
		T *ret = (T*)alloc::Allocate(sizeof(T));
		//���ù��캯��
		new(ret)T();
		return ret;
	}
	//������new[]
	static T* Alloc(size_t n){
		//���ٿռ�
		T *ret =  (T*)alloc::Allocate(sizeof(T)*n);	
		//�������ù��캯��
		for(int i=0; i<n; ++i){
			new(ret+i)T();
		}
		return ret;
	}
	//������delete
	static void DeAlloc(T *p){
		//������������
		p->~T();
		//�ͷſռ�
		alloc::Deallocate(p);
	}
	//������delete[]
	static void DeAlloc(T *p,size_t n){
		//����������������
		for(int i=0; i<n; ++i){
			(p+i)->~T();
		}
		//�ͷſռ�
		alloc::Deallocate(p,n);
	}
};
//����һ���ռ�������
void TestAllocator1(){
	typedef _MallocAllocTemplate<0> alloc;
	
	SimpleAlloc<int,alloc> sa;

	int *p1 = sa.Alloc();		//new
	sa.DeAlloc(p1);				//delete

	int *p2 = sa.Alloc(10);		//new[]
	sa.DeAlloc(p2,10);			//delete[]

} 


int main(){
	TestAllocator1();
	return 0;
}