﻿#ifndef HGL_WORKFLOW_INCLUDE
#define HGL_WORKFLOW_INCLUDE

#include<hgl/thread/Thread.h>
#include<hgl/thread/SwapData.h>
#include<hgl/thread/DataPost.h>
#include<hgl/type/List.h>
namespace hgl
{
	/**
	 * 工作流名字空间<br>
	 * 工作流是一种对工作的多线安排管理机制，它适用于按业务分配多线程的工作环境<br>
	 * 开发者需要为每一种工作指定一定的线程数量，但每一种工作确只有一个工作分配入口和分发出口。<br>
	 * 由其它程序提交工作任务到入口，开发者可以自行重载分配入口的分配函数。
     *
     * 使用方法：
     *
	 *	group=new WorkGroup;			//创建group
     *
	 *	WorkPost *wp[10];
     *
	 *	for(int i=0;i<MAX_THREADS;i++)
	 *	{
	 *		wp=new WorkPost();			//创建事件投递器
	 *		wt=new WorkThread(wp);		//创建工作线程
     *
	 *		group->Add(wp);				//添加投递器到group
	 *		group->Add(wt);				//添加工作线程到group
	 *	}
     *
	 *	group->Start();					//启动group，开启所有工作线程
     *
	 *	for(int i=0;i<0xffff;i++)
	 *	{
	 *		int index=rand()%10;		//随机一个线程
     *
	 *		for(int j=0;j<rand()%10;j++)
	 *		{
	 *			wp[index]->Post(new Work);			//投递一个工作
	 *		}
	 *	}
	 *
	 *	for(int i=0;i<MAX_THREADS;i++)
	 *		wp[i]->ToWork();
     *
	 *	group->Close();					//关闭group,关闭所有工作线程
	 */
	namespace workflow
	{
		/**
		 * 工作线程退出枚举
		 */
		enum WorkThreadExit:uint
		{
			wteNone=0,

			wteFinish,				///<工作结束退出
			wteForce,				///<强制退出

			wteEnd
		};

		template<typename W> class WorkProc
		{
		public:

			virtual ~WorkProc()=default;

		public:	//投递工作线程所需调用的方法

			virtual void Post(W *w)=0;																		///<投递一个工作
			virtual void Post(W **w,int count)=0;															///<投递一批工作

		public:	//需用户重载实现的真正执行工作的方法

			/**
			 * 单个工作执行事件函数，此函数需用户重载实现
			 */
			virtual void OnWork(const uint,W *)=0;

		public:	//由工作线程调用的执行工作事件函数

			virtual bool OnExecuteWork(const uint,const WorkThreadExit)=0;
		};//template<typename W> class WorkProc

		/**
		 * 单体工作处理<br>
		 * 该类可以由多个线程投递工作，但只能被一个工作线程获取工作
		 */
		template<typename W> class SingleWorkProc:public WorkProc<W>
		{
		public:

			using WorkList=List<W *>;

		private:

			SemSwapData<WorkList> work_list;																///<工程列表

		protected:

			double time_out;

		public:

			SingleWorkProc()
			{
				time_out=5;
			}

			virtual ~SingleWorkProc()=default;

			void SetTimeOut(const double to)																///<设置超时时间
			{
				if(to<=0)time_out=0;
					else time_out=to;
			}

			virtual void Post(W *w)																			///<投递一个工作
			{
				WorkList &wl=work_list.GetPost();
					wl.Add(w);
				work_list.ReleasePost();
			}

			virtual void Post(W **w,int count)																///<投递一批工作
			{
				WorkList &wl=work_list.GetPost();
					wl.Add(w,count);
				work_list.ReleasePost();
			}

			virtual void ToWork()																			///<将堆积的工作列表发送给工作线程
			{
				work_list.ReleaseSem(1);
			}

		public:

			/**
			 * 当前工作序列完成事件函数，如需使用请重载此函数
			 */
			virtual void OnFinish(const uint wt_index)
			{
			}

			/**
			 * 开始执行工作函数
			 */
			virtual bool OnExecuteWork(const uint wt_index,const WorkThreadExit wte)
			{
				if(!work_list.WaitSemSwap(time_out))
				{
					if(wte==wteForce||wte==wteFinish)
						return(false);

					return(true);
				}

				WorkList &wl=work_list.GetReceive();

				const int count=wl.GetCount();

				if(count>0)
				{
					W **p=wl.GetData();

					for(int i=0;i<count;i++)
					{
						this->OnWork(wt_index,*p);
						++p;
					}

					this->OnFinish(wt_index);

					wl.ClearData();
				}

				if(wte==wteForce)
					return(false);

				return(true);
			}
		};//template<typename W> class SingleWorkProc:public WorkProc<W>

		/**
		 * 多体工作处理<br>
		 * 该类可以由多个线程投递工作，也可以同时被多个工作线程获取工作
		 */
		template<typename W> class MultiWorkProc:public WorkProc<W>
		{
		protected:

			SemDataPost<W> work_list;																		///<工程列表

		protected:

			double time_out;

		public:

			MultiWorkProc()
			{
				time_out=5;
			}

			virtual ~MultiWorkProc()=default;

			void SetTimeOut(const double to)																///<设置超时时间
			{
				if(to<=0)time_out=0;
					else time_out=to;
			}

			virtual void Post(W *w)																			///<投递一个工作
			{
				if(!w)return;

				work_list.Post(w);
				work_list.ReleaseSem(1);
			}

			virtual void Post(W **w,int count)																///<投递一批工作
			{
				if(!w||count<=0)return;

				work_list.Post(w,count);				
				work_list.ReleaseSem(count);
			}

		public:

			/**
			 * 开始执行工作函数
			 */
			virtual bool OnExecuteWork(const uint wt_index,const WorkThreadExit wte)
			{
				W *obj=work_list.WaitSemSwap(time_out);

				if(!obj)
				{
					if(wte==wteForce||wte==wteFinish)
						return(false);

					return(true);
				}

				this->OnWork(wt_index,obj);

				if(wte==wteForce)
					return(false);

				return(true);
			}			
		};//template<typename W> class MultiWorkProc:public WorkProc<W>

		/**
		 * 工作线程，用于真正处理事务
		 */
		template<typename W> class WorkThread:public Thread
		{
		protected:

			using WorkList=List<W *>;

			WorkProc<W> *work_proc;

			uint work_thread_index;

			atom_uint exit_work;																	///<退出标记

		public:

			WorkThread(WorkProc<W> *wp)
			{
				work_proc=wp;
				work_thread_index=0;
				exit_work=wteNone;
			}

			virtual ~WorkThread()=default;

            bool IsExitDelete()const override{return false;}								///<返回在退出线程时，不删除本对象

			void SetWorkThreadIndex(const uint index)
			{
				work_thread_index=index;
			}

			void ExitWork(WorkThreadExit wte)
			{
				exit_work=wte;
			}

			virtual bool Execute() override
			{
				if(!work_proc)
					RETURN_FALSE;

				return work_proc->OnExecuteWork(work_thread_index,WorkThreadExit(exit_work.operator const hgl::uint()));
			}
		};//template<typename W> class WorkThread:public Thread

		/**
		 * 工作组<br>
		 * 用于管理一组的工作线程以及投递器<br>
		 * 注：可以一组工作线程共用一个投递器，也可以每个工作线程配一个投递器。工作组管理只为方便统一清理
		 */
		template<typename WP,typename WT> class WorkGroup
		{
			ObjectList<WP> wp_list;														///<投递器列表
			ObjectList<WT> wt_list;														///<工作线程列表

		public:

			virtual ~WorkGroup()
			{
				Close();
			}

			virtual bool Add(WP *wp)
			{
				if(!wp)return(false);

				wp_list.Add(wp);
				return(true);
			}

			virtual bool Add(WP **wp,const int count)
			{
				if(!wp)return(false);

				wp_list.Add(wp,count);
				return(true);
			}

			virtual bool Add(WT *wt)
			{
				if(!wt)return(false);

				int index=wt_list.Add(wt);
				wt->SetWorkThreadIndex(index);
				return(true);
			}

			virtual bool Add(WT **wt,const int count)
			{
				if(!wt)return(false);

				int index=wt_list.Add(wt,count);
				for(int i=0;i<count;i++)
				{
					(*wt)->SetWorkThreadIndex(index);
					++index;
					++wt;
				}

				return(true);
			}

			virtual bool Start()
			{
				int count=wt_list.GetCount();

				if(count<=0)
					RETURN_FALSE;

                WT **wt=wt_list.GetData();

				for(int i=0;i<count;i++)
					wt[i]->Start();

				return(true);
			}

			virtual void Close(WorkThreadExit wte=wteFinish)
			{
				int count=wt_list.GetCount();

                WT **wt=wt_list.GetData();

				for(int i=0;i<count;i++)
					wt[i]->ExitWork(wte);

				for(int i=0;i<count;i++)
					wt[i]->Wait();
			}
		};//template<typename WP,typename WT> class WorkGroup
	}//namespace workflow
}//namespace hgl
#endif//HGL_WORKFLOW_INCLUDE