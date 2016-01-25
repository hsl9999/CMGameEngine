﻿#ifndef HGL_GRAPH_OPENGL_CORE_UNIFORM_BUFFER_OBJECT_INCLUDE
#define HGL_GRAPH_OPENGL_CORE_UNIFORM_BUFFER_OBJECT_INCLUDE

#include<hgl/graph/Shader.h>
namespace hgl
{
	namespace graph
	{
		/**
		* OpenGL Core Uniform Buffer Object
		*/
		class UBO:public ShaderDataBlock
		{
		protected:

			unsigned int buffer_id;
			int update_level;

		public:

			UBO(const char *,int,int);
			~UBO();

			void SetData(void *);
			void ChangeData(void *,int,int);
		};//class UBO
	}//namespace graph
}//namespace hgl
#endif//HGL_GRAPH_OPENGL_CORE_UNIFORM_BUFFER_OBJECT_INCLUDE