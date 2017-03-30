/*
*文件说明:测试文件
*作者:高小调
*日期:2017-03-26
*集成开发环境:Microsoft Visual Studio 2010 
*/
#include<iostream>
#include<cstdio>
#include<malloc.h>
using namespace std;
#include"allocator.h"

//对空间配置器的再一层封装
template<typename T,typename Alloc>
class SimpleAlloc{
public:
	//类似于new
	static T* Alloc(){
		//开辟空间
		T *ret = (T*)alloc::Allocate(sizeof(T));
		//调用构造函数
		new(ret)T();
		return ret;
	}
	//类似于new[]
	static T* Alloc(size_t n){
		//开辟空间
		T *ret =  (T*)alloc::Allocate(sizeof(T)*n);	
		//批量调用构造函数
		for(int i=0; i<n; ++i){
			new(ret+i)T();
		}
		return ret;
	}
	//类似于delete
	static void DeAlloc(T *p){
		//调用析构函数
		p->~T();
		//释放空间
		alloc::Deallocate(p);
	}
	//类似于delete[]
	static void DeAlloc(T *p,size_t n){
		//批量调用析构函数
		for(int i=0; i<n; ++i){
			(p+i)->~T();
		}
		//释放空间
		alloc::Deallocate(p,n);
	}
};
//测试一级空间配置器
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