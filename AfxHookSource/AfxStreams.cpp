#include "stdafx.h"

#include "AfxStreams.h"

#include "SourceInterfaces.h"
#include "WrpVEngineClient.h"
#include "csgo_CSkyBoxView.h"
#include "csgo_view.h"
#include "csgo_CViewRender.h"
#include "RenderView.h"
#include "ClientTools.h"
#include "d3d9Hooks.h"
#include "csgo_GlowOverlay.h"
#include "MirvPgl.h"
#include "csgo_Audio.h"

#include <shared/StringTools.h>
#include <shared/FileTools.h>
#include <shared/RawOutput.h>
#include <shared/OpenExrOutput.h>

#include <Windows.h>

#include <stdio.h>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <utility>

#if AFXSTREAMS_REFTRACKER

#include <atomic>

std::atomic_int g_AfxStreams_RefTracker_Count = 0;

void AfxStreams_RefTracker_Inc(void)
{
	++g_AfxStreams_RefTracker_Count;
}

void AfxStreams_RefTracker_Dec(void)
{
	--g_AfxStreams_RefTracker_Count;
}

int AfxStreams_RefTracker_Get(void)
{
	return g_AfxStreams_RefTracker_Count;
}

#endif


extern WrpVEngineClient * g_VEngineClient;
extern SOURCESDK::IMaterialSystem_csgo * g_MaterialSystem_csgo;

IAfxMatRenderContext * GetCurrentContext()
{
	return MatRenderContextHook(g_MaterialSystem_csgo);
}

CAfxStreams g_AfxStreams;

////////////////////////////////////////////////////////////////////////////////

/* Doesn't work for some reason.
void DebugDepthFixDraw(IMesh_csgo * pMesh)
{
	MeshDesc_t_csgo meshDesc;
	VertexDesc_t_csgo vertexDesc;

	int nMaxVertexCount, nMaxIndexCount;
	nMaxVertexCount =  nMaxIndexCount = 4;

	pMesh->SetPrimitiveType( MATERIAL_POINTS );

	pMesh->LockMesh(nMaxVertexCount, nMaxIndexCount, meshDesc, 0);

	IIndexBuffer_csgo * m_pIndexBuffer = pMesh;
	int m_nIndexCount = 0;
	int m_nMaxIndexCount = nMaxIndexCount;

	int m_nIndexOffset = meshDesc.m_nFirstVertex;
	unsigned short * m_pIndices = meshDesc.m_pIndices;
	unsigned int m_nIndexSize = meshDesc.m_nIndexSize;
	int m_nCurrentIndex = 0;

	IVertexBuffer_csgo * m_pVertexBuffer = pMesh;
	memcpy( static_cast<VertexDesc_t_csgo*>( &vertexDesc ), static_cast<const VertexDesc_t_csgo*>( &meshDesc ), sizeof(VertexDesc_t_csgo) );
	int m_nMaxVertexCount = nMaxVertexCount;

	unsigned int m_nTotalVertexCount = 0;
	//unsigned int m_nBufferOffset = static_cast< const VertexDesc_t_csgo* >( &meshDesc )->m_nOffset;
	//unsigned int m_nBufferFirstVertex = meshDesc.m_nFirstVertex;

	int m_nVertexCount = 0;

	int m_nCurrentVertex = 0;

	float * m_pCurrPosition = vertexDesc.m_pPosition;
	float * m_pCurrNormal = vertexDesc.m_pNormal;
	float * m_pCurrTexCoord[VERTEX_MAX_TEXTURE_COORDINATES_csgo];
	for ( size_t i = 0; i < VERTEX_MAX_TEXTURE_COORDINATES_csgo; i++ )
	{
		m_pCurrTexCoord[i] = vertexDesc.m_pTexCoord[i];
	}
	unsigned char * m_pCurrColor = vertexDesc.m_pColor;
	// BEGIN actual "drawing":

	// Position:
	{
		float *pDst = m_pCurrPosition;
		*pDst++ = 0.0f;
		*pDst++ = 0.0f;
		*pDst = 0.0f;
	}

	// Color:
	if(false) {
		int r = 0;
		int g = 0;
		int b = 0;
		int a = 0;

		int col = b | (g << 8) | (r << 16) | (a << 24);

		*(int*)m_pCurrColor = col;
	}


	// Normal:
	{
		float *pDst = m_pCurrNormal;
		*pDst++ = 0.0f;
		*pDst++ = 0.0f;
		*pDst = 0.0f;
	}


	// TextCoord:
	{
		float *pDst = m_pCurrTexCoord[0];
		*pDst++ = 1.0f;
		*pDst++ = 0.0f;
		*pDst = 0.0f;
	}

	// AdvanceVertex:
	{
		if ( ++m_nCurrentVertex > m_nVertexCount )
		{
			m_nVertexCount = m_nCurrentVertex;
		}

		//m_pCurrPosition = reinterpret_cast<float*>( reinterpret_cast<unsigned char*>( m_pCurrPosition ) + vertexDesc.m_VertexSize_Position );
		//m_pCurrColor += vertexDesc.m_VertexSize_Color;
	}

	// End drawing:

	{
		int nIndexCount = 1;
		if(0 != m_nIndexSize)
		{
			int nMaxIndices = m_nMaxIndexCount - m_nCurrentIndex;
			nIndexCount = min( nMaxIndices, nIndexCount );
			if ( nIndexCount != 0 )
			{
				unsigned short *pIndices = &m_pIndices[m_nCurrentIndex];

				//GenerateSequentialIndexBuffer( pIndices, nIndexCount, m_nIndexOffset );
				// What about m_IndexOffset? -> dunno.

				*pIndices = m_nIndexOffset;
			}

			m_nCurrentIndex += nIndexCount * m_nIndexSize;
			if ( m_nCurrentIndex > m_nIndexCount )
			{
				m_nIndexCount = m_nCurrentIndex; 
			}
		}
	}
	
//	Tier0_Msg("Spew: ");
//	pMesh->Spew( m_nVertexCount, m_nIndexCount, meshDesc );

//	pMesh->ValidateData( m_nVertexCount ,m_nIndexCount, meshDesc );

	pMesh->UnlockMesh( m_nVertexCount, m_nIndexCount, meshDesc );

	// Draw!!!!
	pMesh->Draw();
}
*/

void QueueOrExecute(IAfxMatRenderContextOrg * ctx, SOURCESDK::CSGO::CFunctor * functor)
{
	SOURCESDK::CSGO::ICallQueue * queue = ctx->GetCallQueue();

	if (!queue)
{	
		functor->AddRef();
		(*functor)();
		functor->Release();
	}
	else
	{
		queue->QueueFunctor(functor);
	}
}

class AfxD3D9PushOverrideState_Functor
	: public CAfxFunctor
{
public:
	virtual void operator()()
	{
		AfxD3D9PushOverrideState();
	}
};

class AfxD3D9PopOverrideState_Functor
	: public CAfxFunctor
{
public:
	virtual void operator()()
	{
		AfxD3D9PopOverrideState();
	}
};

class AfxD3D9OverrideEnd_ModulationColor_Functor
	: public CAfxFunctor
{
public:
	virtual void operator()()
	{
		AfxD3D9OverrideEnd_ModulationColor();
	}
};

class AfxD3D9OverrideEnd_ModulationBlend_Functor
	: public CAfxFunctor
{
public:
	virtual void operator()()
	{
		AfxD3D9OverrideEnd_ModulationBlend();
	}
};

class AfxD3D9OverrideEnd_D3DRS_ZWRITEENABLE_Functor
	: public CAfxFunctor
{
public:
	virtual void operator()()
	{
		AfxD3D9OverrideEnd_D3DRS_ZWRITEENABLE();
	}
};

class AfxD3D9OverrideBegin_ModulationBlend_Functor
	: public CAfxFunctor
{
public:
	AfxD3D9OverrideBegin_ModulationBlend_Functor(float blend)
		: m_Blend(blend)
	{
	}

	virtual void operator()()
	{
		AfxD3D9OverrideBegin_ModulationBlend(m_Blend);
	}
private:
	float m_Blend;
};

class AfxD3D9OverrideBegin_ModulationColor_Functor
	: public CAfxFunctor
{
public:
	AfxD3D9OverrideBegin_ModulationColor_Functor(float color[3])
	{
		m_Color[0] = color[0];
		m_Color[1] = color[1];
		m_Color[2] = color[2];
	}

	virtual void operator()()
	{
		AfxD3D9OverrideBegin_ModulationColor(m_Color);
	}
private:
	float m_Color[3];
};


class AfxD3D9OverrideBegin_D3DRS_ZWRITEENABLE_Functor
	: public CAfxFunctor
{
public:
	AfxD3D9OverrideBegin_D3DRS_ZWRITEENABLE_Functor(DWORD enable)
		: m_Enable(enable)
	{
	}

	virtual void operator()()
	{
		AfxD3D9OverrideBegin_D3DRS_ZWRITEENABLE(m_Enable);
	}
private:
	DWORD m_Enable;
};

class AfxD3D9OverrideEnd_D3DRS_DESTBLEND_Functor
	: public CAfxFunctor
{
public:
	virtual void operator()()
	{
		AfxD3D9OverrideEnd_D3DRS_DESTBLEND();
	}
};

class AfxD3D9OverrideEnd_D3DRS_SRCBLEND_Functor
	: public CAfxFunctor
{
public:
	virtual void operator()()
	{
		AfxD3D9OverrideEnd_D3DRS_SRCBLEND();
	}
};

class AfxD3D9OverrideEnd_D3DRS_ALPHABLENDENABLE_Functor
	: public CAfxFunctor
{
public:
	virtual void operator()()
	{
		AfxD3D9OverrideEnd_D3DRS_ALPHABLENDENABLE();
	}
};

class AfxD3D9OverrideBegin_D3DRS_ALPHABLENDENABLE_Functor
	: public CAfxFunctor
{
public:
	AfxD3D9OverrideBegin_D3DRS_ALPHABLENDENABLE_Functor(DWORD value)
		: m_Value(value)
	{
	}

	virtual void operator()()
	{
		AfxD3D9OverrideBegin_D3DRS_ALPHABLENDENABLE(m_Value);
	}
private:
	DWORD m_Value;
};

class AfxD3D9OverrideBegin_D3DRS_SRCBLEND_Functor
	: public CAfxFunctor
{
public:
	AfxD3D9OverrideBegin_D3DRS_SRCBLEND_Functor(DWORD value)
		: m_Value(value)
	{
	}

	virtual void operator()()
	{
		AfxD3D9OverrideBegin_D3DRS_SRCBLEND(m_Value);
	}
private:
	DWORD m_Value;
};

class AfxD3D9OverrideBegin_D3DRS_DESTBLEND_Functor
	: public CAfxFunctor
{
public:
	AfxD3D9OverrideBegin_D3DRS_DESTBLEND_Functor(DWORD value)
		: m_Value(value)
	{
	}

	virtual void operator()()
	{
		AfxD3D9OverrideBegin_D3DRS_DESTBLEND(m_Value);
	}
private:
	DWORD m_Value;
};

class AfxD3D9BlockPresent_Functor
	: public CAfxFunctor
{
public:
	AfxD3D9BlockPresent_Functor(bool value)
		: m_Value(value)
	{
	}

	virtual void operator()()
	{
		AfxD3D9_Block_Present(m_Value);
	}

private:
	bool m_Value;
};

#ifdef AFX_MIRV_PGL

class AfxSupplyCamData_Functor
	: public CAfxFunctor
{
public:
	AfxSupplyCamData_Functor(MirvPgl::CamData const & value)
		: m_Value(value)
	{
	}

	virtual void operator()()
	{
		MirvPgl::DrawingThread_SupplyCamData(m_Value);
	}

private:
	MirvPgl::CamData m_Value;
};

#endif

// CAfxFileTracker /////////////////////////////////////////////////////////////

void CAfxFileTracker::TrackFile(char const * filePath)
{
	std::string str(filePath);

	m_FilePaths.push(str);
}

void CAfxFileTracker::WaitForFiles(unsigned int maxUnfinishedFiles)
{
	while(m_FilePaths.size() > maxUnfinishedFiles)
	{
		FILE * file;

		//Tier0_Msg("Waiting for file \"%s\" .... ", m_FilePaths.front().c_str());

		do
		{
			file = fopen(m_FilePaths.front().c_str(), "rb+");
		}while(!file);

		fclose(file);

		//Tier0_Msg("done.\n");

		m_FilePaths.pop();
	}
}

// CAfxRenderViewStream ////////////////////////////////////////////////////////

CAfxRenderViewStream::CAfxRenderViewStream()
: m_DrawViewModel(true)
, m_DrawHud(false)
, m_StreamCaptureType(SCT_Normal)
{
}

CAfxRenderViewStream::~CAfxRenderViewStream()
{
}

char const * CAfxRenderViewStream::AttachCommands_get(void)
{
	return m_AttachCommands.c_str();
}

void CAfxRenderViewStream::AttachCommands_set(char const * value)
{
	m_AttachCommands.assign(value);
}

char const * CAfxRenderViewStream::DetachCommands_get(void)
{
	return m_DetachCommands.c_str();
}

void CAfxRenderViewStream::DetachCommands_set(char const * value)
{
	m_DetachCommands.assign(value);
}

bool CAfxRenderViewStream::DrawHud_get(void)
{
	return m_DrawHud;
}

void CAfxRenderViewStream::DrawHud_set(bool value)
{
	m_DrawHud = value;
}

bool CAfxRenderViewStream::DrawViewModel_get(void)
{
	return m_DrawViewModel;
}

void CAfxRenderViewStream::DrawViewModel_set(bool value)
{
	m_DrawViewModel = value;
}

CAfxRenderViewStream::StreamCaptureType CAfxRenderViewStream::StreamCaptureType_get(void)
{
	return m_StreamCaptureType;
}

void CAfxRenderViewStream::StreamCaptureType_set(StreamCaptureType value)
{
	m_StreamCaptureType = value;
}

void CAfxRenderViewStream::QueueCapture(IAfxMatRenderContextOrg * ctx, CAfxRecordStream * captureTarget, int x, int y, int width, int height)
{
	QueueOrExecute(ctx, new CAfxLeafExecute_Functor(new CCaptureFunctor(*this, captureTarget, x, y, width, height)));
}

void CAfxRenderViewStream::Capture(CAfxRecordStream * captureTarget, int x, int y, int width, int height)
{
	IAfxMatRenderContextOrg * ctx = GetCurrentContext()->GetOrg();

	bool isDepthF = m_StreamCaptureType == CAfxRenderViewStream::SCT_DepthF || m_StreamCaptureType == CAfxRenderViewStream::SCT_DepthFZIP;

	CAfxImageBuffer * buffer = g_AfxStreams.ImageBufferPool.AquireBuffer();

	if(isDepthF)
	{
		if(buffer->AutoRealloc(CAfxImageBuffer::IBPF_ZFloat, width, height))
		{
			unsigned char * pBuffer = (unsigned char*)buffer->Buffer;
			int imagePitch = buffer->ImagePitch;

			ctx->ReadPixels(
				x, y, width, height,
				pBuffer,
				SOURCESDK::IMAGE_FORMAT_R32F
			);

			// Post process buffer:

			float depthScale = 1.0f;
			float depthOfs = 0.0f;

			if(CAfxBaseFxStream * baseFx = this->AsAfxBaseFxStream())
			{
				depthScale = baseFx->DepthValMax_get() - baseFx->DepthVal_get();
				depthOfs = baseFx->DepthVal_get();
			}

			for(int y=0; y < height; ++y)
			{
				for(int x=0; x < width; ++x)
				{
					float depth = *(float *)((unsigned char *)pBuffer +y*imagePitch +x*sizeof(float));

					depth *= depthScale;
					depth += depthOfs;

					*(float *)((unsigned char *)pBuffer +y*imagePitch +x*sizeof(float))
						= depth;
				}
			}

			captureTarget->OnImageBufferCaptured(this, buffer);
		}
		else
		{
			buffer->Release();
			Tier0_Warning("CAfxStreams::CaptureStreamToBuffer: Failed to realloc buffer.\n");
		}
	}
	else
	if(buffer->AutoRealloc(CAfxImageBuffer::IBPF_BGR, width, height))
	{
		unsigned char * pBuffer = (unsigned char*)buffer->Buffer;
		int imagePitch = buffer->ImagePitch;

		ctx->ReadPixels(
			x, y, width, height,
			(unsigned char*)pBuffer,
			SOURCESDK::IMAGE_FORMAT_RGB888
		);

		if(CAfxRenderViewStream::SCT_Depth24 == m_StreamCaptureType || CAfxRenderViewStream::SCT_Depth24ZIP == m_StreamCaptureType)
		{
			float depthScale = 1.0f;
			float depthOfs = 0.0f;

			if(CAfxBaseFxStream * baseFx = this->AsAfxBaseFxStream())
			{
				depthScale = baseFx->DepthValMax_get() - baseFx->DepthVal_get();
				depthOfs = baseFx->DepthVal_get();
			}

			int oldImagePitch =  imagePitch;

			// make the 24bit RGB into a float buffer:
			if(buffer->AutoRealloc(CAfxImageBuffer::IBPF_ZFloat, width, height))
			{
				unsigned char * pBuffer = (unsigned char*)buffer->Buffer;
				int imagePitch = buffer->ImagePitch;

				for(int y = height-1; y >= 0; --y)
				{
					for(int x = width-1; x >= 0; --x)
					{
						unsigned char r = ((unsigned char *)pBuffer)[y*oldImagePitch +3*x +0];
						unsigned char g = ((unsigned char *)pBuffer)[y*oldImagePitch +3*x +1];
						unsigned char b = ((unsigned char *)pBuffer)[y*oldImagePitch +3*x +2];

						float depth;

						depth = (1.0f/16777215.0f)*r +(256.0f/16777215.0f)*g +(65536.0f/16777215.0f)*b;

						depth *= depthScale;
						depth += depthOfs;

						*(float *)((unsigned char *)pBuffer +y*imagePitch +x*sizeof(float))
							= depth;
					}
				}

				captureTarget->OnImageBufferCaptured(this, buffer);
			}
			else
			{
				buffer->Release();
				Tier0_Warning("CAfxStreams::CaptureStreamToBuffer: Failed to realloc buffer.\n");
			}
		}
		else
		{
			// (back) transform to MDT native format:

			int lastLine = height >> 1;
			if(height & 0x1) ++lastLine;

			for(int y=0;y<lastLine;++y)
			{
				int srcLine = y;
				int dstLine = height -1 -y;

				for(int x=0;x<width;++x)
				{
					unsigned char r = ((unsigned char *)pBuffer)[dstLine*imagePitch +3*x +0];
					unsigned char g = ((unsigned char *)pBuffer)[dstLine*imagePitch +3*x +1];
					unsigned char b = ((unsigned char *)pBuffer)[dstLine*imagePitch +3*x +2];
									
					((unsigned char *)pBuffer)[dstLine*imagePitch +3*x +0] = ((unsigned char *)pBuffer)[srcLine*imagePitch +3*x +2];
					((unsigned char *)pBuffer)[dstLine*imagePitch +3*x +1] = ((unsigned char *)pBuffer)[srcLine*imagePitch +3*x +1];
					((unsigned char *)pBuffer)[dstLine*imagePitch +3*x +2] = ((unsigned char *)pBuffer)[srcLine*imagePitch +3*x +0];
									
					((unsigned char *)pBuffer)[srcLine*imagePitch +3*x +0] = b;
					((unsigned char *)pBuffer)[srcLine*imagePitch +3*x +1] = g;
					((unsigned char *)pBuffer)[srcLine*imagePitch +3*x +2] = r;
				}
			}

			captureTarget->OnImageBufferCaptured(this, buffer);
		}
	}
	else
	{
		buffer->Release();
		Tier0_Warning("CAfxStreams::CaptureStreamToBuffer: Failed to realloc buffer.\n");
	}
}

// CAfxRenderViewStream::CCaptureFunctor ///////////////////////////////////////

CAfxRenderViewStream::CCaptureFunctor::CCaptureFunctor(CAfxRenderViewStream & stream, CAfxRecordStream * captureTarget, int x, int y, int width, int height)
	: m_Stream(stream)
	, m_CaptureTarget(captureTarget)
	, m_X(x), m_Y(y), m_Width(width), m_Height(height)
{
	m_Stream.AddRef();
	m_Stream.InterLockIncrement();

	m_CaptureTarget->AddRef();
}

void CAfxRenderViewStream::CCaptureFunctor::operator()()
{
	m_Stream.Capture(m_CaptureTarget, m_X, m_Y, m_Width, m_Height);

	m_CaptureTarget->Release();

	m_Stream.InterLockDecrement();
	m_Stream.Release();
}

// CAfxRecordStream ////////////////////////////////////////////////////////////

CAfxRecordStream::CShared CAfxRecordStream::m_Shared;

CAfxRecordStream::CAfxRecordStream(char const * streamName)
: CAfxStream()
, m_StreamName(streamName)
, m_Record(true)
{

}

bool CAfxRecordStream::Record_get(void)
{
	return m_Record;
}

void CAfxRecordStream::Record_set(bool value)
{
	m_Record = value;
}

void CAfxRecordStream::RecordStart()
{
	m_TriedCreatePath = false;
	m_SucceededCreatePath = false;
}

bool CAfxRecordStream::CreateCapturePath(const std::wstring & takeDir, int frameNumber, wchar_t const * fileExtension, std::wstring &outPath)
{
	if(!m_TriedCreatePath)
	{
		m_TriedCreatePath = true;
		std::wstring wideStreamName;
		if(AnsiStringToWideString(m_StreamName.c_str(), wideStreamName))
		{
			m_CapturePath = takeDir;
			m_CapturePath.append(L"\\");
			m_CapturePath.append(wideStreamName);

			bool dirCreated = CreatePath(m_CapturePath.c_str(), m_CapturePath);
			if(dirCreated)
			{
				m_SucceededCreatePath = true;
			}
			else
			{
				Tier0_Warning("ERROR: could not create \"%s\"\n", m_CapturePath.c_str());
			}
		}
		else
		{
			Tier0_Warning("Error: Failed to convert stream name \"%s\" to a wide string.\n", m_StreamName.c_str());
		}
	}

	if(!m_SucceededCreatePath)
		return false;

	std::wostringstream os;
	os << m_CapturePath << L"\\" << std::setfill(L'0') << std::setw(5) << frameNumber << std::setw(0) << fileExtension;

	outPath = os.str();

	return true;
}

void CAfxRecordStream::RecordEnd()
{

}

char const * CAfxRecordStream::StreamName_get(void)
{
	return m_StreamName.c_str();
}

void CAfxRecordStream::QueueCaptureStart(IAfxMatRenderContextOrg * ctx)
{
	QueueOrExecute(ctx, new CAfxLeafExecute_Functor(new CCaptureStartFunctor(*this)));
}

void CAfxRecordStream::QueueCaptureEnd(IAfxMatRenderContextOrg * ctx, const std::wstring & takeDir, int frameNumber, wchar_t const * fileExtension)
{
	QueueOrExecute(ctx, new CAfxLeafExecute_Functor(new CCaptureEndFunctor(*this, takeDir, frameNumber, fileExtension)));
}


// CAfxSingleStream ////////////////////////////////////////////////////////////

CAfxSingleStream::CAfxSingleStream(char const * streamName, CAfxRenderViewStream * stream)
: CAfxRecordStream(streamName)
, m_Stream(stream)
{
	m_Stream->AddRef();

	m_CaptureCondition.notify_one();
}

CAfxSingleStream::~CAfxSingleStream()
{
	m_Stream->Release();
}

CAfxRenderViewStream * CAfxSingleStream::Stream_get(void)
{
	return m_Stream;
}

void CAfxSingleStream::LevelShutdown(void)
{
	m_Stream->LevelShutdown();
}

void CAfxSingleStream::OnImageBufferCaptured(CAfxRenderViewStream * stream, CAfxImageBuffer * buffer)
{
	if (m_Stream == stream)
		m_Buffer = buffer;
	else
		buffer->Release();
}


void CAfxSingleStream::CaptureStart(void)
{
	{
		std::unique_lock<std::mutex> lock(m_CaptureMutex);

		m_CaptureCondition.wait(lock, [this]() { return !m_Capturing; });

		m_Capturing = true;
	}

	m_Buffer = 0;
}

void CAfxSingleStream::CaptureEnd(std::wstring const * outPath)
{
	CAfxImageBuffer * buffer = m_Buffer;

	{
		std::unique_lock<std::mutex> lock(m_CaptureMutex);
		m_Capturing = false;
	}

	m_CaptureCondition.notify_one();

	if (buffer)
	{
		if (outPath)
		{
			CAfxRenderViewStream::StreamCaptureType captureType = m_Stream->StreamCaptureType_get();

			WriteFile_EnterScope();
			
			if (!buffer->WriteToFile(*outPath, (captureType == CAfxRenderViewStream::SCT_Depth24ZIP || captureType == CAfxRenderViewStream::SCT_DepthFZIP), g_AfxStreams.m_FormatBmpAndNotTga))
			{
				Tier0_Warning("AFXERROR: Failed writing image for stream %s\n.", this->StreamName_get());
			}

			WriteFile_ExitScope();
		}

		buffer->Release();
	}
}


// CAfxTwinStream //////////////////////////////////////////////////////////////

CAfxTwinStream::CAfxTwinStream(char const * streamName, CAfxRenderViewStream * streamA, CAfxRenderViewStream * streamB, StreamCombineType streamCombineType)
: CAfxRecordStream(streamName)
, m_StreamA(streamA)
, m_StreamB(streamB)
, m_StreamCombineType(streamCombineType)
{
	m_StreamA->AddRef();
	m_StreamB->AddRef();
}

CAfxTwinStream::~CAfxTwinStream()
{
	m_StreamB->Release();
	m_StreamA->Release();
}

void CAfxTwinStream::LevelShutdown(void)
{
	m_StreamA->LevelShutdown();
	m_StreamB->LevelShutdown();
}

CAfxRenderViewStream * CAfxTwinStream::StreamA_get()
{
	return m_StreamA;
}

CAfxRenderViewStream * CAfxTwinStream::StreamB_get()
{
	return m_StreamB;
}

CAfxTwinStream::StreamCombineType CAfxTwinStream::StreamCombineType_get(void)
{
	return m_StreamCombineType;
}

void CAfxTwinStream::StreamCombineType_set(StreamCombineType value)
{
	m_StreamCombineType = value;
}

void CAfxTwinStream::OnImageBufferCaptured(CAfxRenderViewStream * stream, CAfxImageBuffer * buffer)
{
	if (m_StreamA == stream)
		m_BufferA = buffer;
	else if (m_StreamB == stream)
		m_BufferB = buffer;
	else
		buffer->Release();
}


void CAfxTwinStream::CaptureStart(void)
{
	{
		std::unique_lock<std::mutex> lock(m_CaptureMutex);

		m_CaptureCondition.wait(lock, [this]() { return !m_Capturing; });

		m_Capturing = true;
	}

	m_BufferA = 0;
	m_BufferB = 0;
}

void CAfxTwinStream::CaptureEnd(std::wstring const * outPath)
{
	CAfxImageBuffer * bufferA = m_BufferA;
	CAfxImageBuffer * bufferB = m_BufferB;
	CAfxRenderViewStream::StreamCaptureType captureType;
	bool canCombine = false;

	if (CAfxTwinStream::SCT_ARedAsAlphaBColor == m_StreamCombineType)
	{
		bufferA = m_BufferB;
		captureType = m_StreamB->StreamCaptureType_get();
		bufferB = m_BufferA;
		canCombine = true;
	}
	else
	if (CAfxTwinStream::SCT_AColorBRedAsAlpha == m_StreamCombineType)
	{
		bufferA = m_BufferA;
		captureType = m_StreamA->StreamCaptureType_get();
		bufferB = m_BufferB;
		canCombine = true;
	}

	{
		std::unique_lock<std::mutex> lock(m_CaptureMutex);
		m_Capturing = false;
	}

	m_CaptureCondition.notify_one();

	if (outPath && canCombine && bufferA && bufferB)
	{
		int orgImagePitch;

		canCombine = 
			bufferA->Width == bufferB->Width
			&& bufferA->Height == bufferB->Height
			&& bufferA->PixelFormat == bufferA->IBPF_BGR
			&& bufferA->PixelFormat == bufferB->PixelFormat
			&& (orgImagePitch = bufferA->ImagePitch) == bufferB->ImagePitch
			&& bufferA->AutoRealloc(bufferA->IBPF_BGRA, bufferA->Width, bufferA->Height)
			;

		if (canCombine)
		{
			// interleave B as alpha into A:

			int height = bufferA->Height;
			int width = bufferA->Width;
			int newImagePitchA = bufferA->ImagePitch;

			unsigned char * pBufferA = (unsigned char *)(bufferA->Buffer);
			unsigned char * pBufferB = (unsigned char *)(bufferB->Buffer);

			for (int y = height - 1;y >= 0;--y)
			{
				for (int x = width - 1;x >= 0;--x)
				{
					unsigned char b = ((unsigned char *)pBufferA)[y*orgImagePitch + x * 3 + 0];
					unsigned char g = ((unsigned char *)pBufferA)[y*orgImagePitch + x * 3 + 1];
					unsigned char r = ((unsigned char *)pBufferA)[y*orgImagePitch + x * 3 + 2];
					unsigned char a = ((unsigned char *)pBufferB)[y*orgImagePitch + x * 3 + 0];

					((unsigned char *)pBufferA)[y*newImagePitchA + x * 4 + 0] = b;
					((unsigned char *)pBufferA)[y*newImagePitchA + x * 4 + 1] = g;
					((unsigned char *)pBufferA)[y*newImagePitchA + x * 4 + 2] = r;
					((unsigned char *)pBufferA)[y*newImagePitchA + x * 4 + 3] = a;
				}
			}

			WriteFile_EnterScope();

			if (!bufferA->WriteToFile(*outPath, (captureType == CAfxRenderViewStream::SCT_Depth24ZIP || captureType == CAfxRenderViewStream::SCT_DepthFZIP), g_AfxStreams.m_FormatBmpAndNotTga))
			{
				Tier0_Warning("AFXERROR: Failed writing image for stream %s\n.", this->StreamName_get());
			}

			WriteFile_ExitScope();
		}
		else
		{
			Tier0_Warning("CAfxStreams::View_Render: Combining sub-streams for stream %s, failed.\n", this->StreamName_get());
		}
	}

	if (bufferB) bufferB->Release();
	if (bufferA) bufferA->Release();
}



// CAfxBaseFxStream ////////////////////////////////////////////////////////////

CAfxBaseFxStream::CShared CAfxBaseFxStream::m_Shared;

CAfxBaseFxStream::CAfxBaseFxStream()
: CAfxRenderViewStream()
, m_MapRleaseNotification(this)
, m_PickerMaterialsRleaseNotification(this)
, m_TestAction(false)
, m_DepthVal(1)
, m_DepthValMax(1024)
, m_SmokeOverlayAlphaFactor(1)
, m_ShouldForceNoVisOverride(false)
, m_DebugPrint(false)
, m_ClientEffectTexturesAction(0)
, m_WorldTexturesAction(0)
, m_SkyBoxTexturesAction(0)
, m_StaticPropTexturesAction(0)
, m_CableAction(0)
, m_PlayerModelsAction(0)
, m_WeaponModelsAction(0)
, m_StatTrakAction(0)
, m_ShellModelsAction(0)
, m_OtherModelsAction(0)
, m_DecalTexturesAction(0)
, m_EffectsAction(0)
, m_ShellParticleAction(0)
, m_OtherParticleAction(0)
, m_StickerAction(0)
, m_ErrorMaterialAction(0)
, m_OtherAction(0)
, m_WriteZAction(0)
, m_DevAction(0)
, m_OtherEngineAction(0)
, m_OtherSpecialAction(0)
, m_VguiAction(0)
{
	m_Shared.AddRef();

	SetAction(m_ClientEffectTexturesAction, m_Shared.DrawAction_get());
	SetAction(m_WorldTexturesAction, m_Shared.DrawAction_get());
	SetAction(m_SkyBoxTexturesAction, m_Shared.DrawAction_get());
	SetAction(m_StaticPropTexturesAction, m_Shared.DrawAction_get());
	SetAction(m_CableAction, m_Shared.DrawAction_get());
	SetAction(m_PlayerModelsAction, m_Shared.DrawAction_get());
	SetAction(m_WeaponModelsAction, m_Shared.DrawAction_get());
	SetAction(m_StatTrakAction, m_Shared.DrawAction_get());
	SetAction(m_ShellModelsAction, m_Shared.DrawAction_get());
	SetAction(m_OtherModelsAction, m_Shared.DrawAction_get());
	SetAction(m_DecalTexturesAction, m_Shared.DrawAction_get());
	SetAction(m_EffectsAction, m_Shared.DrawAction_get());
	SetAction(m_ShellParticleAction, m_Shared.DrawAction_get());
	SetAction(m_OtherParticleAction, m_Shared.DrawAction_get());
	SetAction(m_StickerAction, m_Shared.DrawAction_get());
	SetAction(m_ErrorMaterialAction, m_Shared.DrawAction_get());
	SetAction(m_OtherAction, m_Shared.DrawAction_get());
	SetAction(m_WriteZAction, m_Shared.DrawAction_get());
	SetAction(m_DevAction, m_Shared.DrawAction_get());
	SetAction(m_OtherEngineAction, m_Shared.DrawAction_get());
	SetAction(m_OtherSpecialAction, m_Shared.DrawAction_get());
	SetAction(m_VguiAction, m_Shared.DrawAction_get());
}

CAfxBaseFxStream::~CAfxBaseFxStream()
{
	InvalidateMap();

	SetAction(m_ClientEffectTexturesAction, 0);
	SetAction(m_WorldTexturesAction, 0);
	SetAction(m_SkyBoxTexturesAction, 0);
	SetAction(m_StaticPropTexturesAction, 0);
	SetAction(m_CableAction, 0);
	SetAction(m_PlayerModelsAction, 0);
	SetAction(m_WeaponModelsAction, 0);
	SetAction(m_StatTrakAction, 0);
	SetAction(m_ShellModelsAction, 0);
	SetAction(m_OtherModelsAction, 0);
	SetAction(m_DecalTexturesAction, 0);
	SetAction(m_EffectsAction, 0);
	SetAction(m_ShellParticleAction, 0);
	SetAction(m_OtherParticleAction, 0);
	SetAction(m_StickerAction, 0);
	SetAction(m_ErrorMaterialAction, 0);
	SetAction(m_OtherAction, 0);
	SetAction(m_WriteZAction, 0);
	SetAction(m_DevAction, 0);
	SetAction(m_OtherEngineAction, 0);
	SetAction(m_OtherSpecialAction, 0);
	SetAction(m_VguiAction, 0);

	m_Shared.Release();
}

void CAfxBaseFxStream::AfxStreamsInit(void)
{
	m_Shared.AfxStreamsInit();
}

void CAfxBaseFxStream::AfxStreamsShutdown(void)
{
	m_Shared.AfxStreamsShutdown();
}


void CAfxBaseFxStream::Console_ActionFilter_Add(const char * expression, CAction * action)
{
	InvalidateMap();
	m_ActionFilter.push_back(CActionFilterValue(expression,action));
}

void CAfxBaseFxStream::Console_ActionFilter_AddEx(CAfxStreams * streams, IWrpCommandArgs * args)
{
	CActionFilterValue * value = CActionFilterValue::Console_Parse(streams, args);
	if (value)
	{
		InvalidateMap();
		m_ActionFilter.push_back(*value);
		delete value;
	}
}

void CAfxBaseFxStream::Console_ActionFilter_Print(void)
{
	int id = 0;
	for(std::list<CActionFilterValue>::iterator it = m_ActionFilter.begin(); it != m_ActionFilter.end(); ++it)
	{
		it->Console_Print(id);

		++id;
	}
}

void CAfxBaseFxStream::Console_ActionFilter_Remove(int id)
{
	int curId = 0;
	for(std::list<CActionFilterValue>::iterator it = m_ActionFilter.begin(); it != m_ActionFilter.end(); ++it)
	{
		if(curId == id)
		{
			InvalidateMap();
			m_ActionFilter.erase(it);
			return;
		}

		++curId;
	}

	Tier0_Warning("Error: %i is not a valid actionFilter id!\n", id);
}

void CAfxBaseFxStream::Console_ActionFilter_Move(int id, int moveBeforeId)
{
	CActionFilterValue val;

	if(moveBeforeId < 0 || moveBeforeId > (int)m_ActionFilter.size())
	{
		Tier0_Msg("Error: %i is not in valid range for <targetId>\n");
		return;
	}

	{
		int curId = 0;
		std::list<CActionFilterValue>::iterator it = m_ActionFilter.begin();
		while(curId < id && it != m_ActionFilter.end())
		{
			++curId;
			++it;
		}

		if(it == m_ActionFilter.end())
		{
			Tier0_Warning("Error: %i is not a valid actionFilter id!\n");
			return;
		}

		val = *it;

		InvalidateMap();

		m_ActionFilter.erase(it);
	}

	{
		int curId = 0;
		std::list<CActionFilterValue>::iterator it = m_ActionFilter.begin();
		while(curId < moveBeforeId && it != m_ActionFilter.end())
		{
			++curId;
			++it;
		}

		m_ActionFilter.insert(it,val);
	}
}

void CAfxBaseFxStream::MainThreadInitialize(void)
{
	m_Shared.MainThreadInitialize();
}

void CAfxBaseFxStream::LevelShutdown(void)
{
	Picker_Stop();
	// no resaon to do that anymore // InvalidateMap();
	m_Shared.LevelShutdown();
}

void CAfxBaseFxStream::OnRenderBegin(void)
{
	CAfxRenderViewStream::OnRenderBegin();

	this->InterLockIncrement();

	CAfxBaseFxStreamContextHook * ch = new CAfxBaseFxStreamContextHook(this, 0);

	ch->RenderBegin(GetCurrentContext());
}

void CAfxBaseFxStream::OnRenderEnd()
{
	if (IAfxMatRenderContext * ctx = GetCurrentContext())
	{
		if (IAfxContextHook * hook = ctx->Hook_get())
		{
			hook->RenderEnd();
		}
	}

	CAfxRenderViewStream::OnRenderEnd();
}

CAfxBaseFxStream::CAction * CAfxBaseFxStream::RetrieveAction(SOURCESDK::IMaterial_csgo * material, SOURCESDK::CSGO::CBaseHandle const & entityHandle)
{
	CAction * action = 0;

	if (Picker_GetHidden(entityHandle, material))
	{
		action = GetAction(material, m_Shared.NoDrawAction_get());
	}

	if (!action)
	{
		CAfxTrackedMaterial findKey(material, 0);

		m_MapMutex.lock_shared();

		std::map<CAfxTrackedMaterial, CCacheEntry>::iterator it = m_Map.find(findKey);

		if (it != m_Map.end())
		{
			std::map<SOURCESDK::CSGO::CBaseHandle, CAction *>::iterator itEnt = it->second.EntityActions.find(entityHandle);
			if (itEnt != it->second.EntityActions.end())
				action = itEnt->second;
			else
				action = it->second.DefaultAction;

			m_MapMutex.unlock_shared();
		}
		else
		{
			m_MapMutex.unlock_shared();
			m_MapMutex.lock();

			it = m_Map.find(findKey);

			if (it != m_Map.end())
			{
				std::map<SOURCESDK::CSGO::CBaseHandle, CAction *>::iterator itEnt = it->second.EntityActions.find(entityHandle);
				if (itEnt != it->second.EntityActions.end())
					action = itEnt->second;
				else
					action = it->second.DefaultAction;
			}

			if (!action)
			{
				// determine current action and cache it.

				CCacheEntry & entry = m_Map.emplace(std::piecewise_construct, std::forward_as_tuple(material, &m_MapRleaseNotification),std::forward_as_tuple()).first->second;

				for (std::list<CActionFilterValue>::iterator it = m_ActionFilter.begin(); it != m_ActionFilter.end(); ++it)
				{
					if (it->CalcMatch_Material(material))
					{
						if (it->GetUseHandle())
						{
							SOURCESDK::CSGO::CBaseHandle const & itHandle = it->GetHandle();

							if (entry.EntityActions.end() == entry.EntityActions.find(itHandle))
							{
								CAction * itAction = GetAction(material, it->GetMatchAction());
								itAction->AddRef();
								entry.EntityActions[itHandle] = itAction;

								if (!action && itHandle == entityHandle)
								{
									action = itAction;
								}
							}
						}
						else
						{
							if (!entry.DefaultAction)
							{
								CAction * itAction = GetAction(material, it->GetMatchAction());
								itAction->AddRef();
								entry.DefaultAction = itAction;

								if (!action)
								{
									action = itAction;
								}
							}
						}
						break;
					}
				}

				if (!entry.DefaultAction)
				{
					CAction * itAction = GetAction(material);
					itAction->AddRef();
					entry.DefaultAction = itAction;

					if (!action)
						action = entry.DefaultAction;
				}

				Assert(0 != action);

				if (m_DebugPrint)
				{
					const char * name = material->GetName();
					const char * groupName = material->GetTextureGroupName();
					const char * shaderName = material->GetShaderName();
					bool isErrorMaterial = material->IsErrorMaterial();

					Tier0_Msg("Stream: RetrieveAction: Material action cache miss: \"handle=%i\" \"name=%s\" \"textureGroup=%s\" \"shader=%s\" \"isErrrorMaterial=%u\" -> %s\n"
						, entityHandle.ToInt()
						, name
						, groupName
						, shaderName
						, isErrorMaterial ? 1 : 0
						, action ? action->Key_get().m_Name.c_str() : "(null)");
				}
			}

			m_MapMutex.unlock();
		}
	}

	if(false)
	{
		const char * name = material->GetName();
		const char * groupName = material->GetTextureGroupName();
		const char * shaderName = material->GetShaderName();
		bool isErrorMaterial = material->IsErrorMaterial();

		Tier0_Msg("Info: \"handle=%i\" (not used) \"name=%s\" \"textureGroup=%s\" \"shader=%s\" \"isErrrorMaterial=%u\" -> %s\n"
			, entityHandle.ToInt()
			, name
			, groupName
			, shaderName
			, isErrorMaterial ? 1 : 0
			, action ? action->Key_get().m_Name.c_str() : "(null)");
	}

	return action;
}

CAfxBaseFxStream::CAction * CAfxBaseFxStream::GetAction(SOURCESDK::IMaterial_csgo * material)
{
	const char * groupName =  material->GetTextureGroupName();
	const char * name = material->GetName();
	//const char * shaderName = material->GetShaderName();
	bool isErrorMaterial = material->IsErrorMaterial();

	if(isErrorMaterial)
		return GetAction(material, m_ErrorMaterialAction);
	else
	if(!strcmp("ClientEffect textures", groupName))
		return GetAction(material, m_ClientEffectTexturesAction);
	else
	if(!strcmp("Decal textures", groupName))
		return GetAction(material, m_DecalTexturesAction);
	else
	if(!strcmp("World textures", groupName))
		return GetAction(material, m_WorldTexturesAction);
	else
	if(!strcmp("SkyBox textures", groupName))
		return GetAction(material, m_SkyBoxTexturesAction);
	else
	if(!strcmp("StaticProp textures", groupName))
	{
		if(StringBeginsWith(name, "models/weapons/"))
			return GetAction(material, m_WeaponModelsAction);
		else
			return GetAction(material, m_StaticPropTexturesAction);
	}
	else
	if(!strcmp("Model textures", groupName))
	{
		if(StringBeginsWith(name, "models/player/"))
			return GetAction(material, m_PlayerModelsAction);
		else
		if(StringBeginsWith(name, "models/weapons/"))
		{
			if(StringBeginsWith(name, "models/weapons/stattrack/"))
				return GetAction(material, m_StatTrakAction);
			else
				return GetAction(material, m_WeaponModelsAction);
		}
		else
		if(StringBeginsWith(name, "models/shells/"))
			return GetAction(material, m_ShellModelsAction);
		else
			return GetAction(material, m_OtherModelsAction);
	}
	else
	if(!strcmp("Other textures", groupName)
		||!strcmp("Precached", groupName))
	{
		if(StringBeginsWith(name, "__"))
			return GetAction(material, m_OtherSpecialAction);
		else
		if(StringBeginsWith(name, "cable/"))
			return GetAction(material, m_CableAction);
		else
		if(StringBeginsWith(name, "cs_custom_material_"))
			return GetAction(material, m_WeaponModelsAction);
		else
		if(StringBeginsWith(name, "engine/"))
		{
			if(!strcmp(name, "engine/writez"))
				return GetAction(material, m_WriteZAction);
			else
				return GetAction(material, m_OtherEngineAction);
		}
		else
		if(StringBeginsWith(name, "effects/"))
			return GetAction(material, m_EffectsAction);
		else
		if(StringBeginsWith(name, "dev/"))
			return GetAction(material, m_DevAction);
		else
		if(StringBeginsWith(name, "particle/"))
		{
			if(StringBeginsWith(name, "particle/shells/"))
				return GetAction(material, m_ShellParticleAction);
			else
				return GetAction(material, m_OtherParticleAction);
		}
		else
		if(StringBeginsWith(name, "sticker_"))
			return GetAction(material, m_StickerAction);
		else
		if(StringBeginsWith(name, "vgui"))
			return GetAction(material, m_VguiAction);
		else
			return GetAction(material, m_OtherAction);
	}
	else
	if(!strcmp("Particle textures", groupName))
	{
		if(StringBeginsWith(name, "particle/shells/"))
			return GetAction(material, m_ShellParticleAction);
		else
			return GetAction(material, m_OtherParticleAction);
	}
	else
	if(!strcmp(groupName, "VGUI textures"))
		return GetAction(material, m_VguiAction);

	return GetAction(material, 0);
}

CAfxBaseFxStream::CAction * CAfxBaseFxStream::GetAction(SOURCESDK::IMaterial_csgo * material, CAction * action)
{
	if(!action) action = m_Shared.DrawAction_get();
	action = action->ResolveAction(material);
	return action;
}

CAfxBaseFxStream::CAction * CAfxBaseFxStream::ClientEffectTexturesAction_get(void)
{
	return m_ClientEffectTexturesAction;
}

void CAfxBaseFxStream::ClientEffectTexturesAction_set(CAction * value)
{
	SetActionAndInvalidateMap(m_ClientEffectTexturesAction, value);
}

CAfxBaseFxStream::CAction * CAfxBaseFxStream::WorldTexturesAction_get(void)
{
	return m_WorldTexturesAction;
}

void CAfxBaseFxStream::WorldTexturesAction_set(CAction *  value)
{
	SetActionAndInvalidateMap(m_WorldTexturesAction, value);
}

CAfxBaseFxStream::CAction * CAfxBaseFxStream::SkyBoxTexturesAction_get(void)
{
	return m_SkyBoxTexturesAction;
}

void CAfxBaseFxStream::SkyBoxTexturesAction_set(CAction * value)
{
	SetActionAndInvalidateMap(m_SkyBoxTexturesAction, value);
}

CAfxBaseFxStream::CAction * CAfxBaseFxStream::StaticPropTexturesAction_get(void)
{
	return m_StaticPropTexturesAction;
}

void CAfxBaseFxStream::StaticPropTexturesAction_set(CAction * value)
{
	SetActionAndInvalidateMap(m_StaticPropTexturesAction, value);
}

CAfxBaseFxStream::CAction * CAfxBaseFxStream::CableAction_get(void)
{
	return m_CableAction;
}

void CAfxBaseFxStream::CableAction_set(CAction * value)
{
	SetActionAndInvalidateMap(m_CableAction, value);
}

CAfxBaseFxStream::CAction * CAfxBaseFxStream::PlayerModelsAction_get(void)
{
	return m_PlayerModelsAction;
}

void CAfxBaseFxStream::PlayerModelsAction_set(CAction * value)
{
	SetActionAndInvalidateMap(m_PlayerModelsAction, value);
}

CAfxBaseFxStream::CAction * CAfxBaseFxStream::WeaponModelsAction_get(void)
{
	return m_WeaponModelsAction;
}

void CAfxBaseFxStream::WeaponModelsAction_set(CAction * value)
{
	SetActionAndInvalidateMap(m_WeaponModelsAction, value);
}

CAfxBaseFxStream::CAction * CAfxBaseFxStream::StatTrakAction_get(void)
{
	return m_StatTrakAction;
}

void CAfxBaseFxStream::StatTrakAction_set(CAction * value)
{
	SetActionAndInvalidateMap(m_StatTrakAction, value);
}

CAfxBaseFxStream::CAction * CAfxBaseFxStream::ShellModelsAction_get(void)
{
	return m_ShellModelsAction;
}

void CAfxBaseFxStream::ShellModelsAction_set(CAction * value)
{
	SetActionAndInvalidateMap(m_ShellModelsAction, value);
}

CAfxBaseFxStream::CAction * CAfxBaseFxStream::OtherModelsAction_get(void)
{
	return m_OtherModelsAction;
}

void CAfxBaseFxStream::OtherModelsAction_set(CAction * value)
{
	SetActionAndInvalidateMap(m_OtherModelsAction, value);
}

CAfxBaseFxStream::CAction * CAfxBaseFxStream::DecalTexturesAction_get(void)
{
	return m_DecalTexturesAction;
}

void CAfxBaseFxStream::DecalTexturesAction_set(CAction * value)
{
	SetActionAndInvalidateMap(m_DecalTexturesAction, value);
}

CAfxBaseFxStream::CAction * CAfxBaseFxStream::EffectsAction_get(void)
{
	return m_EffectsAction;
}

void CAfxBaseFxStream::EffectsAction_set(CAction * value)
{
	SetActionAndInvalidateMap(m_EffectsAction, value);
}

CAfxBaseFxStream::CAction * CAfxBaseFxStream::ShellParticleAction_get(void)
{
	return m_ShellParticleAction;
}

void CAfxBaseFxStream::ShellParticleAction_set(CAction * value)
{
	SetActionAndInvalidateMap(m_ShellParticleAction, value);
}

CAfxBaseFxStream::CAction * CAfxBaseFxStream::OtherParticleAction_get(void)
{
	return m_OtherParticleAction;
}

void CAfxBaseFxStream::OtherParticleAction_set(CAction * value)
{
	SetActionAndInvalidateMap(m_OtherParticleAction, value);
}

CAfxBaseFxStream::CAction * CAfxBaseFxStream::StickerAction_get(void)
{
	return m_StickerAction;
}

void CAfxBaseFxStream::StickerAction_set(CAction * value)
{
	SetActionAndInvalidateMap(m_StickerAction, value);
}

CAfxBaseFxStream::CAction * CAfxBaseFxStream::ErrorMaterialAction_get(void)
{
	return m_ErrorMaterialAction;
}

void CAfxBaseFxStream::ErrorMaterialAction_set(CAction * value)
{
	SetActionAndInvalidateMap(m_ErrorMaterialAction, value);
}

CAfxBaseFxStream::CAction * CAfxBaseFxStream::OtherAction_get(void)
{
	return m_OtherAction;
}

void CAfxBaseFxStream::OtherAction_set(CAction * value)
{
	SetActionAndInvalidateMap(m_OtherAction, value);
}

CAfxBaseFxStream::CAction * CAfxBaseFxStream::WriteZAction_get(void)
{
	return m_WriteZAction;
}

void CAfxBaseFxStream::WriteZAction_set(CAction * value)
{
	SetActionAndInvalidateMap(m_WriteZAction, value);
}

CAfxBaseFxStream::CAction * CAfxBaseFxStream::DevAction_get(void)
{
	return m_DevAction;
}

CAfxBaseFxStream::CAction * CAfxBaseFxStream::OtherEngineAction_get(void)
{
	return m_OtherEngineAction;
}

CAfxBaseFxStream::CAction * CAfxBaseFxStream::OtherSpecialAction_get(void)
{
	return m_OtherSpecialAction;
}

CAfxBaseFxStream::CAction * CAfxBaseFxStream::VguiAction_get(void)
{
	return m_VguiAction;
}

bool CAfxBaseFxStream::TestAction_get(void)
{
	return m_TestAction;
}

void CAfxBaseFxStream::TestAction_set(bool value)
{
	InvalidateMap();
	m_TestAction = value;
}

float CAfxBaseFxStream::DepthVal_get(void)
{
	return m_DepthVal;
}

void CAfxBaseFxStream::DepthVal_set(float value)
{
	m_DepthVal = value;
}

float CAfxBaseFxStream::DepthValMax_get(void)
{
	return m_DepthValMax;
}

void CAfxBaseFxStream::DepthValMax_set(float value)
{
	m_DepthValMax = value;
}

float CAfxBaseFxStream::SmokeOverlayAlphaFactor_get(void)
{
	return m_SmokeOverlayAlphaFactor;
}

void CAfxBaseFxStream::SmokeOverlayAlphaFactor_set(float value)
{
	m_SmokeOverlayAlphaFactor = value;
}

bool CAfxBaseFxStream::ShouldForceNoVisOverride_get(void)
{
	return m_ShouldForceNoVisOverride;
}

void CAfxBaseFxStream::ShouldForceNoVisOverride_set(bool value)
{
	m_ShouldForceNoVisOverride = value;
}


bool CAfxBaseFxStream::DebugPrint_get(void)
{
	return m_DebugPrint;
}

void CAfxBaseFxStream::DebugPrint_set(bool value)
{
	m_DebugPrint = value;
}

void CAfxBaseFxStream::InvalidateMap(void)
{
	m_MapMutex.lock();

	if(m_DebugPrint) Tier0_Msg("Stream: Invalidating material cache.\n");

	for(std::map<CAfxTrackedMaterial, CCacheEntry>::iterator it = m_Map.begin(); it != m_Map.end(); ++it)
	{
		for (std::map<SOURCESDK::CSGO::CBaseHandle, CAction *>::iterator itIt = it->second.EntityActions.begin(); itIt != it->second.EntityActions.end(); ++itIt)
		{
			itIt->second->Release();
		}

		it->second.DefaultAction->Release();
	}
	m_Map.clear();

	m_MapMutex.unlock();
}

void CAfxBaseFxStream::Picker_Stop(void)
{
	std::unique_lock<std::shared_timed_mutex> lock(m_PickerMutex);

	if(m_PickerActive)
	{
		m_PickerMaterials.clear();
		m_PickerEntities.clear();

		m_PickerActive = false;

		Tier0_Msg("Picker stopped.\n");
	}
}

void CAfxBaseFxStream::Picker_Print(void)
{
	std::shared_lock<std::shared_timed_mutex> lock(m_PickerMutex);

	Tier0_Msg("---- Materials: ----\n");
	for (std::map<CAfxTrackedMaterial, CPickerMatValue>::iterator it = m_PickerMaterials.begin(); it != m_PickerMaterials.end(); ++it)
	{
		int idx = it->second.Index;

		CAfxTrackedMaterial const & mat = it->first;

		SOURCESDK::IMaterial_csgo * material = mat.GetMaterial();

		Tier0_Msg("\"name=%s\" \"textureGroup=%s\" \"shader=%s\" \"isErrorMaterial=%i\" (%s)\n", material->GetName(), material->GetTextureGroupName(), material->GetShaderName(), material->IsErrorMaterial() ? 1 : 0, m_PickingMaterials && (1 == (idx & 0x1)) ? "hidden" : "visible");
	}
	Tier0_Msg("---- Entities: ----\n");
	for (std::map<int, CPickerEntValue>::iterator it = m_PickerEntities.begin(); it != m_PickerEntities.end(); ++it)
	{
		int idx = it->second.Index;
		Tier0_Msg("\"handle=%i\" (%s)\n", it->first, m_PickingEntities && (1 == (idx & 0x1)) ? "hidden" : "visible");
	}
	Tier0_Msg("---- END ----\n");
}

void CAfxBaseFxStream::Picker_Pick(bool pickEntityNotMaterial, bool wasVisible)
{
	std::unique_lock<std::shared_timed_mutex> lock(m_PickerMutex);

	if (!m_PickerActive)
	{
		m_PickerActive = true;
		m_PickerCollecting = true;
	}
	else
	{
		if (m_PickerCollecting)
			m_PickerCollecting = false;

		if (m_PickingEntities)
		{
			std::set<CAfxMaterialKey> usedMats;
			int index = 0;

			for (std::map<int, CPickerEntValue>::iterator it = m_PickerEntities.begin(); it != m_PickerEntities.end(); )
			{
				int oldIndex = it->second.Index;

				if ((1 == (oldIndex & 0x1)) == wasVisible)
				{
					it = m_PickerEntities.erase(it);
				}
				else
				{
					usedMats.insert(it->second.Materials.begin(), it->second.Materials.end());
					it->second.Index = index;
					++index;
					++it;
				}
			}

			for(std::map<CAfxTrackedMaterial,CPickerMatValue>::iterator it = m_PickerMaterials.begin(); it != m_PickerMaterials.end(); )
			{
				if (usedMats.end() != usedMats.find(it->first))
					++it;
				else
					it = m_PickerMaterials.erase(it);
			}
		}

		if (m_PickingMaterials)
		{
			std::set<int> usedEnts;
			int index = 0;

			for (std::map<CAfxTrackedMaterial, CPickerMatValue>::iterator it = m_PickerMaterials.begin(); it != m_PickerMaterials.end(); )
			{
				int oldIndex = it->second.Index;

				if ((1 == (oldIndex & 0x1)) == wasVisible)
				{
					it = m_PickerMaterials.erase(it);
				}
				else
				{
					usedEnts.insert(it->second.Entities.begin(), it->second.Entities.end());
					it->second.Index = index;
					++index;
					++it;
				}
			}

			for (std::map<int, CPickerEntValue>::iterator it = m_PickerEntities.begin(); it != m_PickerEntities.end(); )
			{
				if (usedEnts.end() != usedEnts.find(it->first))
					++it;
				else
					it = m_PickerEntities.erase(it);
			}
		}

		bool determinedEntities = m_PickerEntities.size() <= 1;
		bool determinedMaterials = m_PickerMaterials.size() <= 1;

		if (pickEntityNotMaterial)
		{
			if (determinedEntities)
			{
				m_PickingEntities = false;
				m_PickingMaterials = false;

				Tier0_Warning("==== Entity%s determined! ====\n", determinedMaterials ? " and Material" : "");
				if (g_VEngineClient && !g_VEngineClient->Con_IsVisible()) g_VEngineClient->ClientCmd_Unrestricted("toggleconsole");
				return;
			}
		}
		else
		{
			if (determinedMaterials)
			{
				m_PickingEntities = false;
				m_PickingMaterials = false;

				Tier0_Warning("==== %sMaterial determined! ====\n", determinedEntities ? "Entity and " : "");
				if (g_VEngineClient && !g_VEngineClient->Con_IsVisible()) g_VEngineClient->ClientCmd_Unrestricted("toggleconsole");
				return;
			}
		}
	}

	m_PickingEntities = pickEntityNotMaterial;
	m_PickingMaterials = !pickEntityNotMaterial;
}

bool CAfxBaseFxStream::Picker_GetHidden(SOURCESDK::CSGO::CBaseHandle const & entityHandle, SOURCESDK::IMaterial_csgo * material)
{
	if (!m_PickerActive)
		return false;

	m_PickerMutex.lock_shared();

	if (!m_PickerActive)
	{
		m_PickerMutex.unlock_shared();
		return false;
	}

	bool uniqueLocked = false;

	if (m_PickerCollecting)
	{
		m_PickerMutex.unlock_shared();
		m_PickerMutex.lock();

		uniqueLocked = true;
	}

	bool hidden = false;

	if (m_PickerActive)
	{
		int ent = entityHandle.ToInt();
		CAfxTrackedMaterial findMat(material, 0);

		if (!m_PickerCollecting)
		{
			if (!hidden && m_PickingMaterials)
			{
				std::map<CAfxTrackedMaterial, CPickerMatValue>::iterator itMat = m_PickerMaterials.find(findMat);
				hidden = (m_PickerMaterials.end() != itMat) && (((itMat->second.Index) & 0x1) == 1);
			}
			if (!hidden && m_PickingEntities)
			{
				std::map<int, CPickerEntValue>::iterator itEnt = m_PickerEntities.find(ent);
				hidden = (m_PickerEntities.end() != itEnt) && (((itEnt->second.Index) & 0x1) == 1);
			}
		}
		else
		{
			std::map<CAfxTrackedMaterial, CPickerMatValue>::iterator itMat = m_PickerMaterials.lower_bound(findMat);
			if (itMat == m_PickerMaterials.end() || (findMat < itMat->first))
			{
				itMat = m_PickerMaterials.emplace_hint(itMat,std::piecewise_construct, std::forward_as_tuple(material, &m_PickerMaterialsRleaseNotification), std::forward_as_tuple(m_PickerMaterials.size(), ent));
			}
			else
			{
				itMat->second.Entities.insert(ent);
			}

			std::map<int, CPickerEntValue>::iterator itEnt = m_PickerEntities.lower_bound(ent);
			if (itEnt == m_PickerEntities.end() || (ent < itEnt->first))
			{
				itEnt = m_PickerEntities.emplace_hint(itEnt, std::piecewise_construct, std::forward_as_tuple(ent), std::forward_as_tuple(this, m_PickerEntities.size(), material));
			}
			else
			{
				std::set<CAfxTrackedMaterial>::iterator itEntMats = itEnt->second.Materials.lower_bound(findMat);
				if (itEntMats == itEnt->second.Materials.end() || (findMat < *itEntMats))
					itEnt->second.Materials.emplace_hint(itEntMats, material, &(itEnt->second));
			}

			hidden =
				(m_PickingEntities && (((itEnt->second.Index) & 0x1) == 1))
				|| (m_PickingMaterials && (((itMat->second.Index) & 0x1) == 1))
				;
		}

	}

	if (!uniqueLocked)
		m_PickerMutex.unlock_shared();
	else
		m_PickerMutex.unlock();

	return hidden;
}

/*
void CAfxBaseFxStream::ConvertStreamDepth(bool to24, bool depth24ZIP)
{
	ConvertDepthActions(to24);

	m_StreamCaptureType =
		to24 ? (depth24ZIP ? SCT_Depth24ZIP : SCT_Depth24) : SCT_Normal;
}

void CAfxBaseFxStream::ConvertDepthActions(bool to24)
{
	InvalidateMap();

	ConvertDepthAction(m_ClientEffectTexturesAction, to24);
	ConvertDepthAction(m_WorldTexturesAction, to24);
	ConvertDepthAction(m_SkyBoxTexturesAction, to24);
	ConvertDepthAction(m_StaticPropTexturesAction, to24);
	ConvertDepthAction(m_CableAction, to24);
	ConvertDepthAction(m_PlayerModelsAction, to24);
	ConvertDepthAction(m_WeaponModelsAction, to24);
	ConvertDepthAction(m_StatTrakAction, to24);
	ConvertDepthAction(m_ShellModelsAction, to24);
	ConvertDepthAction(m_OtherModelsAction, to24);
	ConvertDepthAction(m_DecalTexturesAction, to24);
	ConvertDepthAction(m_EffectsAction, to24);
	ConvertDepthAction(m_ShellParticleAction, to24);
	ConvertDepthAction(m_OtherParticleAction, to24);
	ConvertDepthAction(m_StickerAction, to24);
	ConvertDepthAction(m_ErrorMaterialAction, to24);
	ConvertDepthAction(m_OtherAction, to24);
	ConvertDepthAction(m_WriteZAction, to24);
	ConvertDepthAction(m_DevAction, to24);
	ConvertDepthAction(m_OtherEngineAction, to24);
	ConvertDepthAction(m_OtherSpecialAction, to24);
	ConvertDepthAction(m_VguiAction, to24);
}

void CAfxBaseFxStream::ConvertDepthAction(CAction * & action, bool to24)
{
	if(!action)
		return;

	char const * name = action->Key_get().m_Name.c_str();
	bool isDepth = !strcmp(name, "drawDepth");
	bool isDepth24 = !strcmp(name, "drawDepth24");

	if(to24)
	{
		if(isDepth) SetAction(action, m_Shared.Depth24Action_get());
	}
	else
	{
		if(isDepth24) SetAction(action, m_Shared.DepthAction_get());
	}
}
*/

void CAfxBaseFxStream::SetActionAndInvalidateMap(CAction * & target, CAction * src)
{
	InvalidateMap();
	SetAction(target, src);
}

void CAfxBaseFxStream::SetAction(CAction * & target, CAction * src)
{
	if(target) target->Release();
	if(src) src->AddRef();
	target = src;
}

// CAfxBaseFxStream::CAfxBaseFxStreamContextHook ///////////////////////////////

void CAfxBaseFxStream::CAfxBaseFxStreamContextHook::RenderBegin(IAfxMatRenderContext * ctx)
{
	m_Ctx = ctx;

	if (0 != m_Ctx->Hook_get())
		Tier0_Warning("CAfxBaseFxStream::CAfxBaseFxStreamContextHook::RenderBegin: Error ctx already hooked!\n");

	m_Ctx->Hook_set(this);

	QueueOrExecute(m_Ctx->GetOrg(), new CAfxLeafExecute_Functor(new AfxD3D9PushOverrideState_Functor()));
}

void CAfxBaseFxStream::CAfxBaseFxStreamContextHook::RenderEnd(void)
{
	BindAction(0);

	QueueOrExecute(m_Ctx->GetOrg(), new CAfxLeafExecute_Functor(new AfxD3D9PopOverrideState_Functor()));
	
	m_Ctx->Hook_set(0);

	m_Ctx = 0;

	m_Stream->InterLockDecrement();

	delete this;
}

void CAfxBaseFxStream::CAfxBaseFxStreamContextHook::QueueFunctorInternal(IAfxCallQueue * aq, SOURCESDK::CSGO::CFunctor *pFunctor)
{
	SOURCESDK::CSGO::ICallQueue * q = aq->GetParent();

	CAfxBaseFxStreamContextHook * ch = new CAfxBaseFxStreamContextHook(m_Stream, this);

	m_Stream->InterLockIncrement();
	q->QueueFunctor(new CRenderBeginFunctor(ch));
	q->QueueFunctorInternal(pFunctor);
	q->QueueFunctor(new CRenderEndFunctor());
}

float CAfxBaseFxStream::CAfxBaseFxStreamContextHook::RenderSmokeOverlayAlphaMod(void)
{
	return m_Stream->m_SmokeOverlayAlphaFactor;
}


bool CAfxBaseFxStream::CAfxBaseFxStreamContextHook::ViewRenderShouldForceNoVis(bool orgValue)
{
	return m_Stream->m_ShouldForceNoVisOverride ? true : orgValue;
}

void CAfxBaseFxStream::CAfxBaseFxStreamContextHook::DrawingHudBegin(void)
{
	BindAction(0); // We don't handle HUD atm.
}

void CAfxBaseFxStream::CAfxBaseFxStreamContextHook::DrawingSkyBoxViewBegin(void)
{
	m_DrawingSkyBoxView = true;
}

void CAfxBaseFxStream::CAfxBaseFxStreamContextHook::DrawingSkyBoxViewEnd(void)
{
	m_DrawingSkyBoxView = false;
}

SOURCESDK::IMaterial_csgo * CAfxBaseFxStream::CAfxBaseFxStreamContextHook::MaterialHook(SOURCESDK::IMaterial_csgo * material)
{
	CAction * action = m_Stream->RetrieveAction(
		material,
		GetCurrentEntityHandle()
	);

	BindAction(action);

	if (m_CurrentAction)
		return m_CurrentAction->MaterialHook(this, material);

	return material;
}

void CAfxBaseFxStream::CAfxBaseFxStreamContextHook::DrawInstances(int nInstanceCount, const SOURCESDK::MeshInstanceData_t_csgo *pInstance)
{
	if (m_CurrentAction)
		m_CurrentAction->DrawInstances(this, nInstanceCount, pInstance);
	else
		m_Ctx->GetOrg()->DrawInstances(nInstanceCount, pInstance);
}

void CAfxBaseFxStream::CAfxBaseFxStreamContextHook::Draw(IAfxMesh * am, int firstIndex, int numIndices)
{
	if (m_CurrentAction)
		m_CurrentAction->Draw(this, am, firstIndex, numIndices);
	else
		am->GetParent()->Draw(firstIndex, numIndices);
}

void CAfxBaseFxStream::CAfxBaseFxStreamContextHook::Draw_2(IAfxMesh * am, SOURCESDK::CPrimList_csgo *pLists, int nLists)
{
	if (m_CurrentAction)
		m_CurrentAction->Draw_2(this, am, pLists, nLists);
	else
		am->GetParent()->Draw(pLists, nLists);
}

void CAfxBaseFxStream::CAfxBaseFxStreamContextHook::DrawModulated(IAfxMesh * am, const SOURCESDK::Vector4D_csgo &vecDiffuseModulation, int firstIndex, int numIndices)
{
	if (m_CurrentAction)
		m_CurrentAction->DrawModulated(this, am, vecDiffuseModulation, firstIndex, numIndices);
	else
		am->GetParent()->DrawModulated(vecDiffuseModulation, firstIndex, numIndices);
}

#if AFX_SHADERS_CSGO
void CAfxBaseFxStream::CAfxBaseFxStreamContextHook::SetVertexShader(CAfx_csgo_ShaderState & state)
{
	if (m_CurrentAction)
		m_CurrentAction->SetVertexShader(this, state);
}

void CAfxBaseFxStream::CAfxBaseFxStreamContextHook::SetPixelShader(CAfx_csgo_ShaderState & state)
{
	if (m_CurrentAction)
		m_CurrentAction->SetPixelShader(this, state);
}
#endif

SOURCESDK::CSGO::CBaseHandle const & CAfxBaseFxStream::CAfxBaseFxStreamContextHook::GetCurrentEntityHandle()
{
	if (m_IsRootCtx)
	{
		// If we are on main thread, we can update with the current value, otherwise we have to use the last value.

		SOURCESDK::CSGO::CBaseHandle handle;

		if (SOURCESDK::IViewRender_csgo * view = GetView_csgo())
		{
			if (SOURCESDK::C_BaseEntity_csgo * ce = view->GetCurrentlyDrawingEntity())
			{
				handle.AfxAssign(ce->GetRefEHandle());
			}
		}

		m_CurrentEntityHandle.AfxAssign(handle);
	}

	return m_CurrentEntityHandle;
}

// CAfxBaseFxStream::CAfxBaseFxStreamContextHook::CRenderBeginFunctor //////////

void CAfxBaseFxStream::CAfxBaseFxStreamContextHook::CRenderBeginFunctor::operator()()
{
	if (IAfxMatRenderContext * ctx = GetCurrentContext())
	{
		m_Ch->RenderBegin(ctx);
	}
}

// CAfxBaseFxStream::CAfxBaseFxStreamContextHook::CRenderEndFunctor ////////////

void CAfxBaseFxStream::CAfxBaseFxStreamContextHook::CRenderEndFunctor::operator()()
{
	if (IAfxMatRenderContext * ctx = GetCurrentContext())
	{
		if (IAfxContextHook * hook = ctx->Hook_get())
		{
			hook->RenderEnd();
		}
	}
}

// CAfxBaseFxStream::CActionKey ////////////////////////////////////////////////

void CAfxBaseFxStream::CActionKey::ToLower(void)
{
	std::transform(m_Name.begin(), m_Name.end(), m_Name.begin(), ::tolower);
}

// CAfxBaseFxStream::CShared ///////////////////////////////////////////////////

CAfxBaseFxStream::CShared::CShared()
{
}

CAfxBaseFxStream::CShared::~CShared()
{
}

void CAfxBaseFxStream::CShared::AfxStreamsInit(void)
{
	CreateStdAction(m_DrawAction, CActionKey("draw"), new CAction());
	CreateStdAction(m_NoDrawAction, CActionKey("noDraw"), new CActionNoDraw());

	CreateStdAction(m_DebugDumpAction, CActionKey("debugDump (don't use)"), new CActionDebugDump());

	CreateStdAction(m_DepthAction, CActionKey("drawDepth"), new CActionDebugDepth(m_NoDrawAction));
	// CreateStdAction(m_DepthAction, CActionKey("drawDepth"), new CActionStandardResolve(CActionStandardResolve::RF_DrawDepth, m_NoDrawAction));

	/*
	m_Depth24Action =CreateStdAction(m_Depth24Action, CActionKey("drawDepth24"), new CActionStandardResolve(CActionStandardResolve::RF_DrawDepth24, m_NoDrawAction));
	*/
	{
		CActionReplace * action = new CActionReplace("afx/greenmatte", m_NoDrawAction);
		float color[3] = { 0,1,0 };
		action->OverrideColor(color);
		CreateStdAction(m_MaskAction, CActionKey("mask"), action);
	}
	// CreateStdAction(m_MaskAction, CActionKey("mask"), new CActionStandardResolve(CActionStandardResolve::RF_GreenScreen, m_NoDrawAction));

	{
		CActionReplace * action = new CActionReplace("afx/white", m_NoDrawAction);
		float color[3] = { 1,1,1 };
		action->OverrideColor(color);
		CreateStdAction(m_WhiteAction, CActionKey("white"), action);
	}
	// CreateStdAction(m_WhiteAction, CActionKey("white"), new CActionStandardResolve(CActionStandardResolve::RF_White, m_NoDrawAction));

	{
		CActionReplace * action = new CActionReplace("afx/black", m_NoDrawAction);
		float color[3] = { 0,0,0 };
		action->OverrideColor(color);
		CreateStdAction(m_BlackAction, CActionKey("black"), action);
	}
	// CreateStdAction(m_BlackAction, CActionKey("black"), new CActionStandardResolve(CActionStandardResolve::RF_Black, m_NoDrawAction));

	// legacy actions:
	CreateAction(CActionKey("invisible"), new CActionNoDraw(), true);
	CreateAction(CActionKey("debugDepth"), new CActionDebugDepth(m_NoDrawAction), true);

}

void CAfxBaseFxStream::CShared::AfxStreamsShutdown(void)
{
	for (std::map<CActionKey, CAction *>::iterator it = m_Actions.begin(); it != m_Actions.end(); ++it)
	{
		it->second->Release();
	}

	if (m_DrawAction) m_DrawAction->Release();
	if (m_NoDrawAction) m_NoDrawAction->Release();
	if (m_DebugDumpAction) m_DebugDumpAction->Release();
	if (m_DepthAction) m_DepthAction->Release();
	/*
	if(m_Depth24Action) m_Depth24Action->Release();
	*/
	if (m_MaskAction) m_MaskAction->Release();
	if (m_WhiteAction) m_WhiteAction->Release();
	if (m_BlackAction) m_BlackAction->Release();
}

void CAfxBaseFxStream::CShared::AddRef()
{
	++m_RefCount;
}

void CAfxBaseFxStream::CShared::Release()
{
	--m_RefCount;
}

void CAfxBaseFxStream::CShared::Console_ListActions(void)
{
	for(std::map<CActionKey, CAction *>::iterator it = m_Actions.begin(); it != m_Actions.end(); ++it)
	{
		Tier0_Msg("%s%s\n", it->second->Key_get().m_Name.c_str(), it->second->IsStockAction_get() ? " (stock action)" : "");
	}
}

void CAfxBaseFxStream::CShared::Console_AddReplaceAction(IWrpCommandArgs * args)
{
	int argc = args->ArgC();

	char const * prefix = args->ArgV(0);

	if(3 <= argc)
	{
		char const * actionName = args->ArgV(1);
		char const * materialName = args->ArgV(2);

		CActionKey key(actionName);

		if(!Console_CheckActionKey(key))
			return;

		bool overrideColor = false;
		float color[3];

		bool overrideBlend = false;
		float blend;

		bool overrideDepthWrite = false;
		bool depthWrite;

		for(int i=3; i < argc; ++i)
		{
			char const * argOpt = args->ArgV(i);

			if(StringBeginsWith(argOpt, "overrideColor="))
			{
				argOpt += strlen("overrideColor=");

				std::istringstream iss(argOpt);

				std::string word;

				int j=0;
				while(j<3 && (iss >> word))
				{
					color[j] = (float)atof(word.c_str());
					++j;
				}

				if(3 == j)
				{
					overrideColor = true;
				}
				else
				{
					Tier0_Warning("Error: overrideColor needs 3 values!\n");
					return;
				}
			}
			else
			if(StringBeginsWith(argOpt, "overrideBlend="))
			{
				argOpt += strlen("overrideBlend=");

				overrideBlend = true;
				blend = (float)atof(argOpt);
			}
			else
			if(StringBeginsWith(argOpt, "overrideDepthWrite="))
			{
				argOpt += strlen("overrideDepthWrite=");

				overrideDepthWrite = true;
				depthWrite = 0 != atoi(argOpt);
			}
			else
			{
				Tier0_Warning("Error: invalid option %s\n", argOpt);
				return;
			}
		}

		CActionReplace * replaceAction = new CActionReplace(materialName, m_NoDrawAction);

		if(overrideColor) replaceAction->OverrideColor(color);
		if(overrideBlend) replaceAction->OverrideBlend(blend);
		if(overrideDepthWrite) replaceAction->OverrideDepthWrite(depthWrite);

		CreateAction(key, replaceAction);
	}
	else
	{
		Tier0_Msg(
			"%s <actionName> <materialName> [option]*\n"
			"Options (yes you can specify multiple) can be:\n"
			"\t\"overrideColor=<rF> <gF> <bF>\"- Where <.F> is a floating point value between 0.0 and 1.0\n"
			"\t\"overrideBlend=<bF>\"- Where <bF> is a floating point value between 0.0 and 1.0\n"
			"\t\"overrideDepthWrite=<iF>\"- Where <iF> is 0 (don't write depth) or 1 (write depth)\n"
			,
			prefix);
	}
}

CAfxBaseFxStream::CAction * CAfxBaseFxStream::CShared::GetAction(CActionKey const & key)
{
	CActionKey lowerKey(key, true);
	
	std::map<CActionKey, CAction *>::iterator it = m_Actions.find(lowerKey);
	if(it != m_Actions.end())
	{
		return it->second;
	}

	return 0;
}

bool CAfxBaseFxStream::CShared::RemoveAction(CActionKey const & key)
{
	CActionKey lowerKey(key, true);

	std::map<CActionKey, CAction *>::iterator it = m_Actions.find(lowerKey);
	if(it != m_Actions.end())
	{
		if(!it->second->IsStockAction_get())
		{
			if(1 == it->second->GetRefCount())
			{
				it->second->Release();
				m_Actions.erase(it);
				return true;
			}
			else
				Tier0_Warning("Action cannot be removed due to dependencies.\n");
		}
		else
			Tier0_Warning("Stock actions cannot be removed!\n");		
	}
	else
		Tier0_Warning("Action not found.");

	return false;
}

void CAfxBaseFxStream::CShared::MainThreadInitialize(void)
{
	for (std::map<CActionKey, CAction *>::iterator it = m_Actions.begin(); it != m_Actions.end(); ++it)
	{
		it->second->MainThreadInitialize();
	}
}

void CAfxBaseFxStream::CShared::LevelShutdown(void)
{
	++m_ShutDownLevel;

	if(m_RefCount == m_ShutDownLevel)
	{
		for(std::map<CActionKey, CAction *>::iterator it = m_Actions.begin(); it != m_Actions.end(); ++it)
		{
			it->second->LevelShutdown();
		}
		m_ShutDownLevel = 0;
	}
}

CAfxBaseFxStream::CAction * CAfxBaseFxStream::CShared::DrawAction_get(void)
{
	return m_DrawAction;
}

CAfxBaseFxStream::CAction * CAfxBaseFxStream::CShared::NoDrawAction_get(void)
{
	return m_NoDrawAction;
}

CAfxBaseFxStream::CAction * CAfxBaseFxStream::CShared::DepthAction_get(void)
{
	return m_DepthAction;
}

/*
CAfxBaseFxStream::CAction * CAfxBaseFxStream::CShared::Depth24Action_get(void)
{
	return m_Depth24Action;
}
*/

CAfxBaseFxStream::CAction * CAfxBaseFxStream::CShared::MaskAction_get(void)
{
	return m_MaskAction;
}

CAfxBaseFxStream::CAction * CAfxBaseFxStream::CShared::WhiteAction_get(void)
{
	return m_WhiteAction;
}

CAfxBaseFxStream::CAction * CAfxBaseFxStream::CShared::BlackAction_get(void)
{
	return m_BlackAction;
}

void CAfxBaseFxStream::CShared::CreateStdAction(CAction * & stdTarget, CActionKey const & key, CAction * action)
{
	CreateAction(key, action, true);
	if(action) action->AddRef();
	stdTarget = action;
}

void CAfxBaseFxStream::CShared::CreateAction(CActionKey const & key, CAction * action, bool isStockAction)
{
	if(action)
	{
		action->Key_set(key);
		action->IsStockAction_set(isStockAction);
		action->AddRef();
	}
	CActionKey lowerKey(key, true);
	m_Actions[lowerKey] = action;
}

bool CAfxBaseFxStream::CShared::Console_CheckActionKey(CActionKey & key)
{
	CActionKey lowerKey(key, true);

	if(StringIsEmpty(key.m_Name.c_str()))
	{
		Tier0_Warning("Error: actionName must not be empty!\n");
		return false;
	}

	if(!StringIsAlNum(key.m_Name.c_str()))
	{
		Tier0_Warning("Error: actionName can only contain letters and numbers!\n");
		return false;
	}

	std::map<CActionKey, CAction *>::iterator it = m_Actions.find(lowerKey);

	if(it != m_Actions.end())
	{
		Tier0_Warning("Error: actionName is already in use!\n");
		return false;
	}

	return true;
}

// CAfxBaseFxStream::CActionFilterValue ////////////////////////////////////////

CAfxBaseFxStream::CActionFilterValue * CAfxBaseFxStream::CActionFilterValue::Console_Parse(CAfxStreams * streams, IWrpCommandArgs * args)
{
	if (args->ArgC() <= 1)
	{
		Tier0_Msg(
			"Usage:\n"
			"%s <option> <option> ...\n"
			"\n"
			"<option> can be:\n"
			"\"handle=<handleNumber>\" (of entity, see mirv_listentities)\n"
			"\"name=<wildCardString>\" (of material)\n"
			"\"textureGroup=<wildCardString>\" (of material)\n"
			"\"shader=<wildCardString>\" (of material)\n"
			"\"isErrorMaterial=0|1\" (of material)\n"
			"\"action=<actionName>\"\n"
			"\n"
			"- The action option must be given!\n"
			"- <wildCardString> is a string without quotes, where \\* is the wildcard and \\\\ is \\-"
			"- Any option except action that is not given will be treated as if it doesn't matter for a match."
			, args->ArgV(0)
		);

		return 0;
	}

	bool useHandle = false;
	SOURCESDK::CSGO::CBaseHandle * handle = 0;
	std::string name("\\*");
	std::string textureGroupName("\\*");
	std::string shaderName("\\*");
	TriState isErrorMaterial = TS_DontCare;
	CAction * matchAction = 0;

	for (int i = 1; i < args->ArgC(); ++i)
	{
		std::string sArg(args->ArgV(i));

		size_t posDelimiter = sArg.find_first_of('=');

		std::string sKey = sArg.substr(0, posDelimiter);
		std::string sValue = sArg.substr(posDelimiter + 1);

		char const * key = sKey.c_str();

		if (!_stricmp("handle", key))
		{
			useHandle = true;
			handle = new SOURCESDK::CSGO::CBaseHandle((unsigned long)(atoi(sValue.c_str())));
		}
		else if (!_stricmp("name", key))
		{
			name = sValue;
		}
		else if (!_stricmp("textureGroup", key))
		{
			textureGroupName = sValue;
		}
		else if (!_stricmp("shader", key))
		{
			shaderName = sValue;
		}
		else if (!_stricmp("isErrorMaterial", key))
		{
			bool value = 0 != atoi(sValue.c_str());

			isErrorMaterial = value ? TS_True : TS_False;
		}
		else if (!_stricmp("action", key))
		{
			CAfxBaseFxStream::CAction * action;
			if (streams->Console_ToAfxAction(sValue.c_str(), action))
			{
				matchAction = action;
			}
			else
				return 0;
		}
		else
		{
			Tier0_Warning("Error: %s is not a valid option!\n", sKey.c_str());
			return 0;
		}
	}

	if (!matchAction)
	{
		Tier0_Warning("Error: Action must be given!\n");
		return 0;
	}

	if (!handle)
	{
		handle = new SOURCESDK::CSGO::CBaseHandle();
	}

	CActionFilterValue * result = new CActionFilterValue(useHandle, *handle, name.c_str(), textureGroupName.c_str(), shaderName.c_str(), isErrorMaterial, matchAction);

	delete handle;

	return result;
}

bool CAfxBaseFxStream::CActionFilterValue::CalcMatch_Material(SOURCESDK::IMaterial_csgo * material)
{
	if (!material)
		return false;

	return
		StringWildCard1Matched(m_Name.c_str(), material->GetName())
		&& StringWildCard1Matched(m_TextureGroupName.c_str(), material->GetTextureGroupName())
		&& StringWildCard1Matched(m_ShaderName.c_str(), material->GetShaderName())
		&& (m_IsErrorMaterial == TS_True ? (material->IsErrorMaterial() == true) : (m_IsErrorMaterial == TS_False ? (material->IsErrorMaterial() == false) : true));
}

// CAfxBaseFxStream::CActionDebugDepth /////////////////////////////////////////

CAfxBaseFxStream::CActionDebugDepth::CActionDebugDepth(CAction * fallBackAction)
: CAction()
, m_FallBackAction(fallBackAction)
, m_DebugDepthMaterial(0)
{
	if(fallBackAction) fallBackAction->AddRef();
}

CAfxBaseFxStream::CActionDebugDepth::~CActionDebugDepth()
{
	delete m_DebugDepthMaterial;
	if(m_FallBackAction) m_FallBackAction->Release();
}

CAfxBaseFxStream::CAction * CAfxBaseFxStream::CActionDebugDepth::ResolveAction(SOURCESDK::IMaterial_csgo * material)
{
	bool splinetype = false;
	bool useinstancing = false;

	if(material)
	{
		int numVars = material->ShaderParamCount();
		SOURCESDK::IMaterialVar_csgo ** orgParams = material->GetShaderParams();
		
		SOURCESDK::IMaterialVar_csgo ** params = orgParams;

		for(int i=0; i<numVars; ++i)
		{
			if(params[0]->IsDefined())
			{
				char const * varName = params[0]->GetName();

				if(!strcmp(varName,"$splinetype"))
				{
					if(params[0]->GetIntValue())
						splinetype = true;
				}
				else
				if(!strcmp(varName,"$useinstancing"))
				{
					if(params[0]->GetIntValue())
						useinstancing = true;
				}
			}

			++params;
		}
	}

	if(splinetype || useinstancing)
		return SafeSubResolveAction(m_FallBackAction, material);

	return this;
}

void CAfxBaseFxStream::CActionDebugDepth::MainThreadInitialize(void)
{
	if(!m_DebugDepthMaterial)
	{
		m_DebugDepthMaterial = new CAfxOwnedMaterial(g_AfxStreams.GetMaterialSystem()->FindMaterial("afx/depth", 0));
	}
}

void CAfxBaseFxStream::CActionDebugDepth::AfxUnbind(CAfxBaseFxStreamContextHook * ch)
{
}

SOURCESDK::IMaterial_csgo * CAfxBaseFxStream::CActionDebugDepth::MaterialHook(CAfxBaseFxStreamContextHook * ch, SOURCESDK::IMaterial_csgo * material)
{
	if (!m_DebugDepthMaterial)		
		return material; // Should not happen, but you never know.

	float scale = ch->DrawingSkyBoxView_get() ? csgo_CSkyBoxView_GetScale() : 1.0f;
	float flDepthFactor = scale * ch->GetStream()->m_DepthVal;
	float flDepthFactorMax = scale * ch->GetStream()->m_DepthValMax;

	QueueOrExecute(ch->GetCtx()->GetOrg(), new CAfxLeafExecute_Functor(new CDepthValFunctor(flDepthFactor, flDepthFactorMax)));

	return m_DebugDepthMaterial->GetMaterial();
}

CAfxBaseFxStream::CActionDebugDepth::CStatic CAfxBaseFxStream::CActionDebugDepth::m_Static;

void CAfxBaseFxStream::CActionDebugDepth::CStatic::SetDepthVal(float min, float max)
{
	m_MatDebugDepthValsMutex.lock();

	if (!m_MatDebugDepthVal) m_MatDebugDepthVal = new WrpConVarRef("mat_debugdepthval");
	if (!m_MatDebugDepthValMax) m_MatDebugDepthValMax = new WrpConVarRef("mat_debugdepthvalmax");

	m_MatDebugDepthVal->SetValueFastHack(min);
	m_MatDebugDepthValMax->SetValueFastHack(max);

	m_MatDebugDepthValsMutex.unlock();
}

// CAfxBaseFxStream::CActionDebugDump //////////////////////////////////////////

void CAfxBaseFxStream::CActionDebugDump::AfxUnbind(CAfxBaseFxStreamContextHook * ch)
{
	g_AfxStreams.DebugDump(ch->GetCtx()->GetOrg());
}

// CAfxBaseFxStream::CActionReplace ////////////////////////////////////////////

CAfxBaseFxStream::CActionReplace::CActionReplace(
	char const * materialName,
	CAction * fallBackAction)
: CAction()
, m_Material(0)
, m_MaterialName(materialName)
, m_OverrideColor(false)
, m_OverrideBlend(false)
, m_OverrideDepthWrite(false)
{
	if(fallBackAction) fallBackAction->AddRef();
	m_FallBackAction = fallBackAction;
}

CAfxBaseFxStream::CActionReplace::~CActionReplace()
{
	if(m_FallBackAction) m_FallBackAction->Release();
	if(m_Material) delete m_Material;
}

CAfxBaseFxStream::CAction * CAfxBaseFxStream::CActionReplace::ResolveAction(SOURCESDK::IMaterial_csgo * material)
{
	if (m_Material)
	{
		bool srcSplinetype;
		bool srcUseinstancing;

		bool dstSplinetype;
		bool dstUseinstancing;

		ExamineMaterial(material, srcSplinetype, srcUseinstancing);
		ExamineMaterial(m_Material->GetMaterial(), dstSplinetype, dstUseinstancing);

		if (srcSplinetype == dstSplinetype
			&& srcUseinstancing == dstUseinstancing)
			return this;
	}

	return SafeSubResolveAction(m_FallBackAction, material);
}

void CAfxBaseFxStream::CActionReplace::MainThreadInitialize(void)
{
	if (!m_Material)
	{
		m_Material = new CAfxOwnedMaterial(g_AfxStreams.GetMaterialSystem()->FindMaterial(m_MaterialName.c_str(), 0));
	}
}

void CAfxBaseFxStream::CActionReplace::AfxUnbind(CAfxBaseFxStreamContextHook * ch)
{
	IAfxMatRenderContext * ctx = ch->GetCtx();
	IAfxContextHook * ctxh = ctx->Hook_get();

	if (m_OverrideColor)
		QueueOrExecute(ctx->GetOrg(), new CAfxLeafExecute_Functor(new AfxD3D9OverrideEnd_ModulationColor_Functor()));

	if (m_OverrideBlend)
		QueueOrExecute(ctx->GetOrg(), new CAfxLeafExecute_Functor(new AfxD3D9OverrideEnd_ModulationBlend_Functor()));

	if (m_OverrideDepthWrite)
		QueueOrExecute(ctx->GetOrg(), new CAfxLeafExecute_Functor(new AfxD3D9OverrideEnd_D3DRS_ZWRITEENABLE_Functor()));
}

SOURCESDK::IMaterial_csgo * CAfxBaseFxStream::CActionReplace::MaterialHook(CAfxBaseFxStreamContextHook * ch, SOURCESDK::IMaterial_csgo * material)
{
	IAfxMatRenderContext * ctx = ch->GetCtx();
	IAfxContextHook * ctxh = ctx->Hook_get();

	if (m_OverrideBlend)
		QueueOrExecute(ctx->GetOrg(), new CAfxLeafExecute_Functor(new AfxD3D9OverrideBegin_ModulationBlend_Functor(m_Blend)));

	if (m_OverrideColor)
		QueueOrExecute(ctx->GetOrg(), new CAfxLeafExecute_Functor(new AfxD3D9OverrideBegin_ModulationColor_Functor(m_Color)));

	if(m_OverrideDepthWrite)
		QueueOrExecute(ctx->GetOrg(), new CAfxLeafExecute_Functor(new AfxD3D9OverrideBegin_D3DRS_ZWRITEENABLE_Functor(m_DepthWrite ? TRUE : FALSE)));

	return m_Material ? m_Material->GetMaterial() : material;
}

void CAfxBaseFxStream::CActionReplace::ExamineMaterial(SOURCESDK::IMaterial_csgo * material, bool & outSplinetype, bool & outUseinstancing)
{
	bool splinetype = false;
	bool useinstancing = false;

	if(material)
	{
		int numVars = material->ShaderParamCount();
		SOURCESDK::IMaterialVar_csgo ** orgParams = material->GetShaderParams();
		
		SOURCESDK::IMaterialVar_csgo ** params = orgParams;

		for(int i=0; i<numVars; ++i)
		{
			if(params[0]->IsDefined())
			{
				char const * varName = params[0]->GetName();

				if(!strcmp(varName,"$splinetype"))
				{
					if(params[0]->GetIntValue())
						splinetype = true;
				}
				else
				if(!strcmp(varName,"$useinstancing"))
				{
					if(params[0]->GetIntValue())
						useinstancing = true;
				}
			}

			++params;
		}
	}

	outSplinetype = splinetype;
	outUseinstancing = useinstancing;
}

#if AFX_SHADERS_CSGO
// CAfxBaseFxStream::CActionStandardResolve:CShared ////////////////////////////

CAfxBaseFxStream::CActionStandardResolve::CShared::CShared()
: m_RefCount(0)
, m_ShutDownLevel(0)
{
	m_StdDepthAction = new CActionUnlitGenericFallback(
		CActionAfxVertexLitGenericHookKey(ShaderCombo_afxHook_vertexlit_and_unlit_generic_ps30::AFXMODE_0, 0.7f)
		, "afx/white"
		);
	m_StdDepthAction->AddRef();
	m_StdDepthAction->Key_set(CActionKey("drawDepth (std)"));

	m_StdDepth24Action = new CActionUnlitGenericFallback(
		CActionAfxVertexLitGenericHookKey(ShaderCombo_afxHook_vertexlit_and_unlit_generic_ps30::AFXMODE_1, 0.7f)
		, "afx/white"
		);
	m_StdDepth24Action->AddRef();
	m_StdDepth24Action->Key_set(CActionKey("drawDepth24 (std)"));

	m_StdMatteAction = new CActionUnlitGenericFallback(
		CActionAfxVertexLitGenericHookKey(ShaderCombo_afxHook_vertexlit_and_unlit_generic_ps30::AFXMODE_2, 0.7f)
		, "afx/white"
		);
	m_StdMatteAction->AddRef();
	m_StdMatteAction->Key_set(CActionKey("mask (std)"));
		
	m_StdBlackAction = new CActionUnlitGenericFallback(
		CActionAfxVertexLitGenericHookKey(ShaderCombo_afxHook_vertexlit_and_unlit_generic_ps30::AFXMODE_3, 0.7f)
		, "afx/white"
		);
	m_StdBlackAction->AddRef();
	m_StdBlackAction->Key_set(CActionKey("white (std)"));

	m_StdWhiteAction = new CActionUnlitGenericFallback(
		CActionAfxVertexLitGenericHookKey(ShaderCombo_afxHook_vertexlit_and_unlit_generic_ps30::AFXMODE_4, 0.7f)
		, "afx/white"
		);
	m_StdWhiteAction->AddRef();
	m_StdWhiteAction->Key_set(CActionKey("white (std)"));
}

CAfxBaseFxStream::CActionStandardResolve::CShared::~CShared()
{
	InvalidateSplineRopeHookActions();
	InvalidateSpritecardHookActions();
	InvalidateVertexLitGenericHookActions();

	if(m_StdWhiteAction) m_StdWhiteAction->Release();
	if(m_StdBlackAction) m_StdBlackAction->Release();
	if(m_StdMatteAction) m_StdMatteAction->Release();
	if(m_StdDepth24Action) m_StdDepth24Action->Release();
	if(m_StdDepthAction) m_StdDepthAction->Release();
}

void CAfxBaseFxStream::CActionStandardResolve::CShared::AddRef()
{
	++m_RefCount;
}

void CAfxBaseFxStream::CActionStandardResolve::CShared::Release()
{
	--m_RefCount;
}

void CAfxBaseFxStream::CActionStandardResolve::CShared::LevelShutdown(IAfxStreams4Stream * streams)
{
	++m_ShutDownLevel;

	if(m_RefCount == m_ShutDownLevel)
	{
		for(std::map<CActionAfxSplineRopeHookKey, CActionAfxSplineRopeHook *>::iterator it = m_SplineRopeHookActions.begin(); it != m_SplineRopeHookActions.end(); ++it)
		{
			it->second->LevelShutdown(streams);
		}
		InvalidateSplineRopeHookActions();

		for(std::map<CActionAfxSpritecardHookKey, CActionAfxSpritecardHook *>::iterator it = m_SpritecardHookActions.begin(); it != m_SpritecardHookActions.end(); ++it)
		{
			it->second->LevelShutdown(streams);
		}
		InvalidateSpritecardHookActions();

		for(std::map<CActionAfxVertexLitGenericHookKey, CActionAfxVertexLitGenericHook *>::iterator it = m_VertexLitGenericHookActions.begin(); it != m_VertexLitGenericHookActions.end(); ++it)
		{
			it->second->LevelShutdown(streams);
		}
		InvalidateVertexLitGenericHookActions();

		if(m_StdWhiteAction) m_StdWhiteAction->LevelShutdown(streams);
		if(m_StdBlackAction) m_StdBlackAction->LevelShutdown(streams);
		if(m_StdMatteAction) m_StdMatteAction->LevelShutdown(streams);
		if(m_StdDepth24Action) m_StdDepth24Action->LevelShutdown(streams);
		if(m_StdDepthAction) m_StdDepthAction->LevelShutdown(streams);

		m_ShutDownLevel = 0;
	}
}

CAfxBaseFxStream::CAction * CAfxBaseFxStream::CActionStandardResolve::CShared::GetStdDepthAction()
{
	return m_StdDepthAction;
}

CAfxBaseFxStream::CAction * CAfxBaseFxStream::CActionStandardResolve::CShared::GetStdDepth24Action()
{
	return m_StdDepth24Action;
}

CAfxBaseFxStream::CAction * CAfxBaseFxStream::CActionStandardResolve::CShared::GetStdMatteAction()
{
	return m_StdMatteAction;
}

CAfxBaseFxStream::CAction * CAfxBaseFxStream::CActionStandardResolve::CShared::GetStdBlackAction()
{
	return m_StdBlackAction;
}

CAfxBaseFxStream::CAction * CAfxBaseFxStream::CActionStandardResolve::CShared::GetStdWhiteAction()
{
	return m_StdWhiteAction;
}

CAfxBaseFxStream::CAction * CAfxBaseFxStream::CActionStandardResolve::CShared::GetSplineRopeHookAction(CActionAfxSplineRopeHookKey & key)
{
	std::map<CActionAfxSplineRopeHookKey, CActionAfxSplineRopeHook *>::iterator it = m_SplineRopeHookActions.find(key);

	if(it != m_SplineRopeHookActions.end())
	{
		return it->second;
	}

	CActionAfxSplineRopeHook * action = new CActionAfxSplineRopeHook(key);
	{
		std::ostringstream ossName;

		ossName << "splineRopeHook(";

		switch(key.AFXMODE)
		{
		case ShaderCombo_afxHook_splinerope_ps20b::AFXMODE_0:
			ossName << "drawDepth";
			break;
		case ShaderCombo_afxHook_splinerope_ps20b::AFXMODE_1:
			ossName << "drawDepth24";
			break;
		case ShaderCombo_afxHook_splinerope_ps20b::AFXMODE_2:
			ossName << "mask";
			break;
		case ShaderCombo_afxHook_splinerope_ps20b::AFXMODE_3:
			ossName << "white";
			break;
		case ShaderCombo_afxHook_splinerope_ps20b::AFXMODE_4:
			ossName << "black";
			break;
		default:
			ossName << "(unknown)";
			break;
		}

		ossName << ", " << key.AlphaTestReference << ")";

		action->Key_set(CActionKey(ossName.str().c_str()));
	}
	action->IsStockAction_set(true);
	action->AddRef();
	m_SplineRopeHookActions[key] = action;

	return action;
}

CAfxBaseFxStream::CAction * CAfxBaseFxStream::CActionStandardResolve::CShared::GetSpritecardHookAction(CActionAfxSpritecardHookKey & key)
{
	std::map<CActionAfxSpritecardHookKey, CActionAfxSpritecardHook *>::iterator it = m_SpritecardHookActions.find(key);

	if(it != m_SpritecardHookActions.end())
	{
		return it->second;
	}

	CActionAfxSpritecardHook * action = new CActionAfxSpritecardHook(key);
	{
		std::ostringstream ossName;

		ossName << "spriteCardHook(";

		switch(key.AFXMODE)
		{
		case ShaderCombo_afxHook_spritecard_ps20b::AFXMODE_0:
			ossName << "drawDepth";
			break;
		case ShaderCombo_afxHook_spritecard_ps20b::AFXMODE_1:
			ossName << "drawDepth24";
			break;
		case ShaderCombo_afxHook_spritecard_ps20b::AFXMODE_2:
			ossName << "mask";
			break;
		case ShaderCombo_afxHook_spritecard_ps20b::AFXMODE_3:
			ossName << "white";
			break;
		case ShaderCombo_afxHook_spritecard_ps20b::AFXMODE_4:
			ossName << "black";
			break;
		default:
			ossName << "(unknown)";
			break;
		}

		ossName << ", " << key.AlphaTestReference << ")";

		action->Key_set(CActionKey(ossName.str().c_str()));
	}
	action->IsStockAction_set(true);
	action->AddRef();
	m_SpritecardHookActions[key] = action;

	return action;
}

CAfxBaseFxStream::CAction * CAfxBaseFxStream::CActionStandardResolve::CShared::GetVertexLitGenericHookAction(CActionAfxVertexLitGenericHookKey & key)
{
	std::map<CActionAfxVertexLitGenericHookKey, CActionAfxVertexLitGenericHook *>::iterator it = m_VertexLitGenericHookActions.find(key);

	if(it != m_VertexLitGenericHookActions.end())
	{
		return it->second;
	}

	CActionAfxVertexLitGenericHook * action = new CActionAfxVertexLitGenericHook(key);
	{
		std::ostringstream ossName;

		ossName << "vertexLitGenericHook(";

		switch(key.AFXMODE)
		{
		case ShaderCombo_afxHook_vertexlit_and_unlit_generic_ps30::AFXMODE_0:
			ossName << "drawDepth";
			break;
		case ShaderCombo_afxHook_vertexlit_and_unlit_generic_ps30::AFXMODE_1:
			ossName << "drawDepth24";
			break;
		case ShaderCombo_afxHook_vertexlit_and_unlit_generic_ps30::AFXMODE_2:
			ossName << "mask";
			break;
		case ShaderCombo_afxHook_vertexlit_and_unlit_generic_ps30::AFXMODE_3:
			ossName << "white";
			break;
		case ShaderCombo_afxHook_vertexlit_and_unlit_generic_ps30::AFXMODE_4:
			ossName << "black";
			break;
		default:
			ossName << "(unknown)";
			break;
		}

		ossName << ", " << key.AlphaTestReference << ")";

		action->Key_set(CActionKey(ossName.str().c_str()));
	}
	action->IsStockAction_set(true);
	action->AddRef();
	m_VertexLitGenericHookActions[key] = action;

	return action;
}

void CAfxBaseFxStream::CActionStandardResolve::CShared::InvalidateSplineRopeHookActions()
{
	for(std::map<CActionAfxSplineRopeHookKey, CActionAfxSplineRopeHook *>::iterator it = m_SplineRopeHookActions.begin();
		it != m_SplineRopeHookActions.end();
		++it)
	{
		it->second->Release();
	}
	m_SplineRopeHookActions.clear();
}


void CAfxBaseFxStream::CActionStandardResolve::CShared::InvalidateSpritecardHookActions()
{
	for(std::map<CActionAfxSpritecardHookKey, CActionAfxSpritecardHook *>::iterator it = m_SpritecardHookActions.begin();
		it != m_SpritecardHookActions.end();
		++it)
	{
		it->second->Release();
	}
	m_SpritecardHookActions.clear();
}

void CAfxBaseFxStream::CActionStandardResolve::CShared::InvalidateVertexLitGenericHookActions()
{
	for(std::map<CActionAfxVertexLitGenericHookKey, CActionAfxVertexLitGenericHook *>::iterator it = m_VertexLitGenericHookActions.begin();
		it != m_VertexLitGenericHookActions.end();
		++it)
	{
		it->second->Release();
	}
	m_VertexLitGenericHookActions.clear();
}


// CAfxBaseFxStream::CActionStandardResolve ////////////////////////////////////

CAfxBaseFxStream::CActionStandardResolve::CShared CAfxBaseFxStream::CActionStandardResolve::m_Shared;

CAfxBaseFxStream::CActionStandardResolve::CActionStandardResolve(ResolveFor resolveFor, CAction * fallBackAction)
: CAction()
, m_ResolveFor(resolveFor)
, m_FallBackAction(fallBackAction)
{
	m_Shared.AddRef();

	if(fallBackAction) fallBackAction->AddRef();
}

CAfxBaseFxStream::CActionStandardResolve::~CActionStandardResolve()
{
	if(m_FallBackAction) m_FallBackAction->Release();

	m_Shared.Release();
}

CAfxBaseFxStream::CAction * CAfxBaseFxStream::CActionStandardResolve::ResolveAction(IMaterial_csgo * material)
{
	const char * shaderName = material->GetShaderName();

	int numVars = material->ShaderParamCount();
	IMaterialVar_csgo ** orgParams = material->GetShaderParams();

	int flags = orgParams[FLAGS]->GetIntValue();
	bool isAlphatest = 0 != (flags & MATERIAL_VAR_ALPHATEST);
	bool isTranslucent = 0 != (flags & MATERIAL_VAR_TRANSLUCENT);
	bool isAdditive = 0 != (flags & MATERIAL_VAR_ADDITIVE);

	float alphaTestReference = 0.7f;
	bool alphaTestReferenceDefined = false;
	bool isPhong = false;
	bool isBump = false;
	bool isSpline = false;
	bool isUseInstancing = false;

	{
		IMaterialVar_csgo ** params = orgParams;

		//Tier0_Msg("---- Params ----\n");

		for(int i=0; i<numVars; ++i)
		{
			//Tier0_Msg("Param: %s -> %s (%s,isTexture: %s)\n",params[0]->GetName(), params[0]->GetStringValue(), params[0]->IsDefined() ? "defined" : "UNDEFINED", params[0]->IsTexture() ? "Y" : "N");

			if(params[0]->IsDefined())
			{
				char const * varName = params[0]->GetName();

				if(!strcmp(varName,"$bumpmap"))
				{
					if(params[0]->IsTexture())
						isBump = true;
				}
				else
				if(!strcmp(varName,"$phong"))
				{
					if(params[0]->GetIntValue())
						isPhong = true;
				}
				else
				if(!strcmp(params[0]->GetName(),"$alphatestreference") && 0.0 < params[0]->GetFloatValue())
				{
					alphaTestReferenceDefined = true;
					alphaTestReference = params[0]->GetFloatValue();
					break;
				}
				else
				if(!strcmp(varName,"$splinetype"))
				{
					if(params[0]->GetIntValue())
						isSpline = true;
				}
				else
				if(!strcmp(varName,"$useinstancing"))
				{
					if(params[0]->GetIntValue())
						isUseInstancing = true;
				}
			}

			++params;
		}
	}

	if(!isAdditive && !strcmp(shaderName, "SplineRope"))
	{
		switch(m_ResolveFor)
		{
		case RF_DrawDepth:
			{
				CActionAfxSplineRopeHookKey key(
					ShaderCombo_afxHook_splinerope_ps20b::AFXMODE_0,
					alphaTestReference);

				return SafeSubResolveAction(m_Shared.GetSplineRopeHookAction(key), material);
			}
		case RF_DrawDepth24:
			{
				CActionAfxSplineRopeHookKey key(
					ShaderCombo_afxHook_splinerope_ps20b::AFXMODE_1,
					alphaTestReference);

				return SafeSubResolveAction(m_Shared.GetSplineRopeHookAction(key), material);
			}
		case RF_GreenScreen:
			{
				CActionAfxSplineRopeHookKey key(
					ShaderCombo_afxHook_splinerope_ps20b::AFXMODE_2,
					alphaTestReference);

				return SafeSubResolveAction(m_Shared.GetSplineRopeHookAction(key), material);
			}
		case RF_Black:
			{
				CActionAfxSplineRopeHookKey key(
					ShaderCombo_afxHook_splinerope_ps20b::AFXMODE_3,
					alphaTestReference);

				return SafeSubResolveAction(m_Shared.GetSplineRopeHookAction(key), material);
			}
		case RF_White:
			{
				CActionAfxSplineRopeHookKey key(
					ShaderCombo_afxHook_splinerope_ps20b::AFXMODE_4,
					alphaTestReference);

				return SafeSubResolveAction(m_Shared.GetSplineRopeHookAction(key), material);
			}
		}
	}
	else
	if(!strcmp(shaderName, "Spritecard"))
	{
		switch(m_ResolveFor)
		{
		case RF_DrawDepth:
			{
				CActionAfxSpritecardHookKey key(
					ShaderCombo_afxHook_spritecard_ps20b::AFXMODE_0,
					alphaTestReference
					);

				return SafeSubResolveAction(m_Shared.GetSpritecardHookAction(key), material);
			}
		case RF_DrawDepth24:
			{
				CActionAfxSpritecardHookKey key(
					ShaderCombo_afxHook_spritecard_ps20b::AFXMODE_1,
					alphaTestReference
					);

				return SafeSubResolveAction(m_Shared.GetSpritecardHookAction(key), material);
			}
		case RF_GreenScreen:
			{
				CActionAfxSpritecardHookKey key(
					ShaderCombo_afxHook_spritecard_ps20b::AFXMODE_2,
					alphaTestReference
					);

				return SafeSubResolveAction(m_Shared.GetSpritecardHookAction(key), material);
			}
		case RF_Black:
			{
				CActionAfxSpritecardHookKey key(
					ShaderCombo_afxHook_spritecard_ps20b::AFXMODE_3,
					alphaTestReference
					);

				return SafeSubResolveAction(m_Shared.GetSpritecardHookAction(key), material);
			}
		case RF_White:
			{
				CActionAfxSpritecardHookKey key(
					ShaderCombo_afxHook_spritecard_ps20b::AFXMODE_4,
					alphaTestReference
					);

				return SafeSubResolveAction(m_Shared.GetSpritecardHookAction(key), material);
			}
		}
	}
	else
	if(!isAdditive && (!strcmp(shaderName, "VertexLitGeneric") || !strcmp(shaderName, "UnlitGeneric")))
	{
		if(!isPhong && !isBump)
		{
			switch(m_ResolveFor)
			{
			case RF_DrawDepth:
				{
					CActionAfxVertexLitGenericHookKey key(
						ShaderCombo_afxHook_vertexlit_and_unlit_generic_ps30::AFXMODE_0,
						alphaTestReference);

					return SafeSubResolveAction(m_Shared.GetVertexLitGenericHookAction(key), material);
				}
			case RF_DrawDepth24:
				{
					CActionAfxVertexLitGenericHookKey key(
						ShaderCombo_afxHook_vertexlit_and_unlit_generic_ps30::AFXMODE_1,
						alphaTestReference);

					return SafeSubResolveAction(m_Shared.GetVertexLitGenericHookAction(key), material);
				}
			case RF_GreenScreen:
				{
					CActionAfxVertexLitGenericHookKey key(
						ShaderCombo_afxHook_vertexlit_and_unlit_generic_ps30::AFXMODE_2,
						alphaTestReference);

					return SafeSubResolveAction(m_Shared.GetVertexLitGenericHookAction(key), material);
				}
			case RF_Black:
				{
					CActionAfxVertexLitGenericHookKey key(
						ShaderCombo_afxHook_vertexlit_and_unlit_generic_ps30::AFXMODE_3,
						alphaTestReference);

					return SafeSubResolveAction(m_Shared.GetVertexLitGenericHookAction(key), material);
				}
			case RF_White:
				{
					CActionAfxVertexLitGenericHookKey key(
						ShaderCombo_afxHook_vertexlit_and_unlit_generic_ps30::AFXMODE_4,
						alphaTestReference);

					return SafeSubResolveAction(m_Shared.GetVertexLitGenericHookAction(key), material);
				}
			}
		}
	}
	else
	if(!strcmp(shaderName, "Sprite_DX9"))
	{
		// cant really handle that shader atm, so use fallback:
		return SafeSubResolveAction(m_FallBackAction, material);
	}

	// Std fallback:

	if(!isAdditive && !isSpline && !isUseInstancing)
	{
		switch(m_ResolveFor)
		{
		case RF_DrawDepth:
			return SafeSubResolveAction(m_Shared.GetStdDepthAction(), material);
		case RF_DrawDepth24:
			return SafeSubResolveAction(m_Shared.GetStdDepth24Action(), material);
		case RF_GreenScreen:
			return SafeSubResolveAction(m_Shared.GetStdMatteAction(), material);
		case RF_Black:
			return SafeSubResolveAction(m_Shared.GetStdBlackAction(), material);
		case RF_White:
			return SafeSubResolveAction(m_Shared.GetStdWhiteAction(), material);
		};
	}

	// total fallback:

	return SafeSubResolveAction(m_FallBackAction, material);
}
#endif

// CAfxBaseFxStream::CActionNoDraw /////////////////////////////////////////////

void CAfxBaseFxStream::CActionNoDraw::AfxUnbind(CAfxBaseFxStreamContextHook * ch)
{
	IAfxMatRenderContext * ctx = ch->GetCtx();
	IAfxContextHook * ctxh = ctx->Hook_get();

	QueueOrExecute(ctx->GetOrg(), new CAfxLeafExecute_Functor(new AfxD3D9OverrideEnd_D3DRS_ZWRITEENABLE_Functor()));
	QueueOrExecute(ctx->GetOrg(), new CAfxLeafExecute_Functor(new AfxD3D9OverrideEnd_D3DRS_DESTBLEND_Functor()));
	QueueOrExecute(ctx->GetOrg(), new CAfxLeafExecute_Functor(new AfxD3D9OverrideEnd_D3DRS_SRCBLEND_Functor()));
	QueueOrExecute(ctx->GetOrg(), new CAfxLeafExecute_Functor(new AfxD3D9OverrideEnd_D3DRS_ALPHABLENDENABLE_Functor()));
}

SOURCESDK::IMaterial_csgo * CAfxBaseFxStream::CActionNoDraw::MaterialHook(CAfxBaseFxStreamContextHook * ch, SOURCESDK::IMaterial_csgo * material)
{
	IAfxMatRenderContext * ctx = ch->GetCtx();
	IAfxContextHook * ctxh = ctx->Hook_get();

	QueueOrExecute(ctx->GetOrg(), new CAfxLeafExecute_Functor(new AfxD3D9OverrideBegin_D3DRS_ALPHABLENDENABLE_Functor(TRUE)));
	QueueOrExecute(ctx->GetOrg(), new CAfxLeafExecute_Functor(new AfxD3D9OverrideBegin_D3DRS_SRCBLEND_Functor(D3DBLEND_ZERO)));
	QueueOrExecute(ctx->GetOrg(), new CAfxLeafExecute_Functor(new AfxD3D9OverrideBegin_D3DRS_DESTBLEND_Functor(D3DBLEND_ONE)));
	QueueOrExecute(ctx->GetOrg(), new CAfxLeafExecute_Functor(new AfxD3D9OverrideBegin_D3DRS_ZWRITEENABLE_Functor(FALSE)));

	return material;
}

// CAfxBaseFxStream::CActionAfxVertexLitGenericHook ////////////////////////////

#if AFX_SHADERS_CSGO
csgo_Stdshader_dx9_Combos_vertexlit_and_unlit_generic_ps20 CAfxBaseFxStream::CActionAfxVertexLitGenericHook::m_Combos_ps20;
csgo_Stdshader_dx9_Combos_vertexlit_and_unlit_generic_ps20b CAfxBaseFxStream::CActionAfxVertexLitGenericHook::m_Combos_ps20b;
csgo_Stdshader_dx9_Combos_vertexlit_and_unlit_generic_ps30 CAfxBaseFxStream::CActionAfxVertexLitGenericHook::m_Combos_ps30;

CAfxBaseFxStream::CActionAfxVertexLitGenericHook::CActionAfxVertexLitGenericHook(CActionAfxVertexLitGenericHookKey & key)
: CAction()
, m_Key(key)
{
}

void CAfxBaseFxStream::CActionAfxVertexLitGenericHook::AfxUnbind(IAfxMatRenderContext * ctx)
{
	AfxD3D9_OverrideEnd_ps_c5();

	AfxD3D9OverrideEnd_D3DRS_SRGBWRITEENABLE();

	//AfxD3D9OverrideEnd_D3DRS_MULTISAMPLEANTIALIAS();

	AfxD3D9_OverrideEnd_SetPixelShader();
}

IMaterial_csgo * CAfxBaseFxStream::CActionAfxVertexLitGenericHook::MaterialHook(IAfxMatRenderContext * ctx, IMaterial_csgo * material)
{
	// depth factors:

	float scale = g_bIn_csgo_CSkyBoxView_Draw ? csgo_CSkyBoxView_GetScale() : 1.0f;
	float flDepthFactor = scale * CAfxBaseFxStream::m_Shared.m_ActiveBaseFxStream->m_DepthVal;
	float flDepthFactorMax = scale * CAfxBaseFxStream::m_Shared.m_ActiveBaseFxStream->m_DepthValMax;

	// Foce multisampling off for depth24:
	//if(m_Key.AFXMODE == ShaderCombo_afxHook_vertexlit_and_unlit_generic_ps30::AFXMODE_1)
	//	AfxD3D9OverrideBegin_D3DRS_MULTISAMPLEANTIALIAS(FALSE);

	// Force SRGBWriteEnable to off:
	AfxD3D9OverrideBegin_D3DRS_SRGBWRITEENABLE(FALSE);

	// Fill in g_AfxConstants in shader:

	float mulFac = flDepthFactorMax -flDepthFactor;
	mulFac = !mulFac ? 0.0f : 1.0f / mulFac;

	float overFac[4] = { flDepthFactor, mulFac, m_Key.AlphaTestReference, 0.0f };

	AfxD3D9_OverrideBegin_ps_c5(overFac);

	// Bind normal material:
	return material;
}

void CAfxBaseFxStream::CActionAfxVertexLitGenericHook::SetPixelShader(CAfx_csgo_ShaderState & state)
{
	char const * shaderName = state.Static.SetPixelShader.pFileName.c_str();

	if(!strcmp(shaderName,"vertexlit_and_unlit_generic_ps20"))
	{
		static bool firstPass = true;
		if(firstPass)
		{
			firstPass = false;
			Tier0_Warning("AFXWARNING: You are using an untested code path in CAfxBaseFxStream::CActionAfxVertexLitGenericHook::SetPixelShader for %s.\n", shaderName);
		}

		m_Combos_ps20.CalcCombos(state.Static.SetPixelShader.nStaticPshIndex, state.Dynamic.SetPixelShaderIndex.pshIndex);

		if(0 < m_Combos_ps20.m_LIGHTNING_PREVIEW)
		{
			Tier0_Warning("AFXERROR: CAfxBaseFxStream::CActionAfxVertexLitGenericHook::SetPixelShader: 0 < m_LIGHTNING_PREVIEW not supported for %s.\n", shaderName);
			return;
		}

		int combo = ShaderCombo_afxHook_vertexlit_and_unlit_generic_ps20::GetCombo(
			(ShaderCombo_afxHook_vertexlit_and_unlit_generic_ps20::AFXMODE_e)m_Key.AFXMODE,
			(ShaderCombo_afxHook_vertexlit_and_unlit_generic_ps20::DETAILTEXTURE_e)m_Combos_ps20.m_DETAILTEXTURE,
			!m_Combos_ps20.m_BASEALPHAENVMAPMASK && !m_Combos_ps20.m_SELFILLUM ? ShaderCombo_afxHook_vertexlit_and_unlit_generic_ps20::NOT_BASEALPHAENVMAPMASK_AND_NOT_SELFILLUM_1 : ShaderCombo_afxHook_vertexlit_and_unlit_generic_ps20::NOT_BASEALPHAENVMAPMASK_AND_NOT_SELFILLUM_0,
			(ShaderCombo_afxHook_vertexlit_and_unlit_generic_ps20::FLASHLIGHT_e)m_Combos_ps20.m_FLASHLIGHT,
			(ShaderCombo_afxHook_vertexlit_and_unlit_generic_ps20::DETAIL_BLEND_MODE_e)m_Combos_ps20.m_DETAIL_BLEND_MODE,
			(ShaderCombo_afxHook_vertexlit_and_unlit_generic_ps20::DESATURATEWITHBASEALPHA_e)m_Combos_ps20.m_DESATURATEWITHBASEALPHA,
			ShaderCombo_afxHook_vertexlit_and_unlit_generic_ps20::NOT_LIGHTING_PREVIEW_ONLY_0
		);

		IAfxPixelShader * afxPixelShader = g_AfxShaders.GetAcsPixelShader("afxHook_vertexlit_and_unlit_generic_ps20.acs", combo);

		if(!afxPixelShader->GetPixelShader())
			Tier0_Warning("AFXERROR: CAfxBaseFxStream::CActionAfxVertexLitGenericHook::SetPixelShader: Replacement Shader combo %i for %s is null.\n", combo, shaderName);
		else
		{
			// Override shader:
			AfxD3D9_OverrideBegin_SetPixelShader(afxPixelShader->GetPixelShader());
		}

		afxPixelShader->Release();
	}
	else
	if(!strcmp(shaderName,"vertexlit_and_unlit_generic_ps20b"))
	{
		static bool firstPass = true;
		if(firstPass)
		{
			firstPass = false;
			Tier0_Warning("AFXWARNING: You are using an untested code path in CAfxBaseFxStream::CActionAfxVertexLitGenericHook::SetPixelShader for %s.\n", shaderName);
		}

		m_Combos_ps20b.CalcCombos(state.Static.SetPixelShader.nStaticPshIndex, state.Dynamic.SetPixelShaderIndex.pshIndex);

		if(0 < m_Combos_ps20b.m_LIGHTNING_PREVIEW)
		{
			Tier0_Warning("AFXERROR: CAfxBaseFxStream::CActionAfxVertexLitGenericHook::SetPixelShader: 0 < m_LIGHTNING_PREVIEW not supported for %s.\n", shaderName);
			return;
		}

		int combo = ShaderCombo_afxHook_vertexlit_and_unlit_generic_ps20b::GetCombo(
			(ShaderCombo_afxHook_vertexlit_and_unlit_generic_ps20b::AFXMODE_e)m_Key.AFXMODE,
			(ShaderCombo_afxHook_vertexlit_and_unlit_generic_ps20b::DETAILTEXTURE_e)m_Combos_ps20b.m_DETAILTEXTURE,
			!m_Combos_ps20b.m_BASEALPHAENVMAPMASK && !m_Combos_ps20b.m_SELFILLUM ? ShaderCombo_afxHook_vertexlit_and_unlit_generic_ps20b::NOT_BASEALPHAENVMAPMASK_AND_NOT_SELFILLUM_1 : ShaderCombo_afxHook_vertexlit_and_unlit_generic_ps20b::NOT_BASEALPHAENVMAPMASK_AND_NOT_SELFILLUM_0,
			(ShaderCombo_afxHook_vertexlit_and_unlit_generic_ps20b::FLASHLIGHT_e)m_Combos_ps20b.m_FLASHLIGHT,
			(ShaderCombo_afxHook_vertexlit_and_unlit_generic_ps20b::DETAIL_BLEND_MODE_e)m_Combos_ps20b.m_DETAIL_BLEND_MODE,
			(ShaderCombo_afxHook_vertexlit_and_unlit_generic_ps20b::DESATURATEWITHBASEALPHA_e)m_Combos_ps20b.m_DESATURATEWITHBASEALPHA,
			ShaderCombo_afxHook_vertexlit_and_unlit_generic_ps20b::NOT_LIGHTING_PREVIEW_ONLY_0
		);

		IAfxPixelShader * afxPixelShader = g_AfxShaders.GetAcsPixelShader("afxHook_vertexlit_and_unlit_generic_ps20b.acs", combo);

		if(!afxPixelShader->GetPixelShader())
			Tier0_Warning("AFXERROR: CAfxBaseFxStream::CActionAfxVertexLitGenericHook::SetPixelShader: Replacement Shader combo %i for %s is null.\n", combo, shaderName);
		else
		{
			// Override shader:
			AfxD3D9_OverrideBegin_SetPixelShader(afxPixelShader->GetPixelShader());
		}

		afxPixelShader->Release();
	}
	else
	if(!strcmp(shaderName,"vertexlit_and_unlit_generic_ps30"))
	{
		m_Combos_ps30.CalcCombos(state.Static.SetPixelShader.nStaticPshIndex, state.Dynamic.SetPixelShaderIndex.pshIndex);

		if(0 < m_Combos_ps30.m_LIGHTNING_PREVIEW)
		{
			Tier0_Warning("AFXERROR: CAfxBaseFxStream::CActionAfxVertexLitGenericHook::SetPixelShader: 0 < m_LIGHTNING_PREVIEW not supported for %s.\n", shaderName);
			return;
		}

		int combo = ShaderCombo_afxHook_vertexlit_and_unlit_generic_ps30::GetCombo(
			m_Key.AFXMODE,
			(ShaderCombo_afxHook_vertexlit_and_unlit_generic_ps30::DETAILTEXTURE_e)m_Combos_ps30.m_DETAILTEXTURE,
			!m_Combos_ps30.m_BASEALPHAENVMAPMASK && !m_Combos_ps30.m_SELFILLUM ? ShaderCombo_afxHook_vertexlit_and_unlit_generic_ps30::NOT_BASEALPHAENVMAPMASK_AND_NOT_SELFILLUM_1 : ShaderCombo_afxHook_vertexlit_and_unlit_generic_ps30::NOT_BASEALPHAENVMAPMASK_AND_NOT_SELFILLUM_0,
			(ShaderCombo_afxHook_vertexlit_and_unlit_generic_ps30::FLASHLIGHT_e)m_Combos_ps30.m_FLASHLIGHT,
			(ShaderCombo_afxHook_vertexlit_and_unlit_generic_ps30::DETAIL_BLEND_MODE_e)m_Combos_ps30.m_DETAIL_BLEND_MODE,
			(ShaderCombo_afxHook_vertexlit_and_unlit_generic_ps30::DESATURATEWITHBASEALPHA_e)m_Combos_ps30.m_DESATURATEWITHBASEALPHA,
			ShaderCombo_afxHook_vertexlit_and_unlit_generic_ps30::NOT_LIGHTING_PREVIEW_ONLY_0
		);

		/*
		Tier0_Msg("%s %i %i - %i - %i %i %i %i %i %i -> %i\n",
			shaderName,
			state.Static.SetPixelShader.nStaticPshIndex, state.Dynamic.SetPixelShaderIndex.pshIndex,
			m_Key.AFXMODE,
			(ShaderCombo_afxHook_vertexlit_and_unlit_generic_ps30::DETAILTEXTURE_e)m_Combos_ps30.m_DETAILTEXTURE,
			!m_Combos_ps30.m_BASEALPHAENVMAPMASK && !m_Combos_ps30.m_SELFILLUM ? ShaderCombo_afxHook_vertexlit_and_unlit_generic_ps30::NOT_BASEALPHAENVMAPMASK_AND_NOT_SELFILLUM_1 : ShaderCombo_afxHook_vertexlit_and_unlit_generic_ps30::NOT_BASEALPHAENVMAPMASK_AND_NOT_SELFILLUM_0,
			(ShaderCombo_afxHook_vertexlit_and_unlit_generic_ps30::FLASHLIGHT_e)m_Combos_ps30.m_FLASHLIGHT,
			(ShaderCombo_afxHook_vertexlit_and_unlit_generic_ps30::DETAIL_BLEND_MODE_e)m_Combos_ps30.m_DETAIL_BLEND_MODE,
			(ShaderCombo_afxHook_vertexlit_and_unlit_generic_ps30::DESATURATEWITHBASEALPHA_e)m_Combos_ps30.m_DESATURATEWITHBASEALPHA,
			ShaderCombo_afxHook_vertexlit_and_unlit_generic_ps30::NOT_LIGHTING_PREVIEW_ONLY_0,
			combo
			);
		*/

		IAfxPixelShader * afxPixelShader = g_AfxShaders.GetAcsPixelShader("afxHook_vertexlit_and_unlit_generic_ps30.acs", combo);

		if(!afxPixelShader->GetPixelShader())
			Tier0_Warning("AFXERROR: CAfxBaseFxStream::CActionAfxVertexLitGenericHook::SetPixelShader: Replacement Shader combo %i for %s is null.\n", combo, shaderName);
		else
		{
			// Override shader:
			AfxD3D9_OverrideBegin_SetPixelShader(afxPixelShader->GetPixelShader());
		}

		afxPixelShader->Release();
	}
	else
		Tier0_Warning("AFXERROR: CAfxBaseFxStream::CActionAfxVertexLitGenericHook::SetPixelShader: No replacement defined for %s.\n", shaderName);

}

// CAfxBaseFxStream::CActionAfxSpritecardHook ////////////////////////////

csgo_Stdshader_dx9_Combos_splinecard_vs20 CAfxBaseFxStream::CActionAfxSpritecardHook::m_Combos_splinecard_vs20;
csgo_Stdshader_dx9_Combos_spritecard_vs20 CAfxBaseFxStream::CActionAfxSpritecardHook::m_Combos_spritecard_vs20;
csgo_Stdshader_dx9_Combos_spritecard_ps20 CAfxBaseFxStream::CActionAfxSpritecardHook::m_Combos_ps20;
csgo_Stdshader_dx9_Combos_spritecard_ps20b CAfxBaseFxStream::CActionAfxSpritecardHook::m_Combos_ps20b;

CAfxBaseFxStream::CActionAfxSpritecardHook::CActionAfxSpritecardHook( CActionAfxSpritecardHookKey & key)
: CAction()
, m_Key(key)
{
}

void CAfxBaseFxStream::CActionAfxSpritecardHook::AfxUnbind(IAfxMatRenderContext * ctx)
{
	AfxD3D9_OverrideEnd_ps_c31();

	AfxD3D9OverrideEnd_D3DRS_SRGBWRITEENABLE();
	AfxD3D9OverrideEnd_D3DRS_DESTBLEND();
	AfxD3D9OverrideEnd_D3DRS_SRCBLEND();
	
	//AfxD3D9OverrideEnd_D3DRS_MULTISAMPLEANTIALIAS();

	AfxD3D9_OverrideEnd_SetPixelShader();
	AfxD3D9_OverrideEnd_SetVertexShader();
}

IMaterial_csgo * CAfxBaseFxStream::CActionAfxSpritecardHook::MaterialHook(IAfxMatRenderContext * ctx, IMaterial_csgo * material)
{
	// depth factors:

	float scale = g_bIn_csgo_CSkyBoxView_Draw ? csgo_CSkyBoxView_GetScale() : 1.0f;
	float flDepthFactor = scale * CAfxBaseFxStream::m_Shared.m_ActiveBaseFxStream->m_DepthVal;
	float flDepthFactorMax = scale * CAfxBaseFxStream::m_Shared.m_ActiveBaseFxStream->m_DepthValMax;

	// Force wanted state:

	// Foce multisampling off for depth24:
	//if(m_Key.AFXMODE == ShaderCombo_afxHook_spritecard_ps20b::AFXMODE_1)
	//	AfxD3D9OverrideBegin_D3DRS_MULTISAMPLEANTIALIAS(FALSE);

	AfxD3D9OverrideBegin_D3DRS_SRCBLEND(D3DBLEND_SRCALPHA);
	AfxD3D9OverrideBegin_D3DRS_DESTBLEND(D3DBLEND_INVSRCALPHA);
	AfxD3D9OverrideBegin_D3DRS_SRGBWRITEENABLE(FALSE);

	// Fill in g_AfxConstants in shader:

	float mulFac = flDepthFactorMax -flDepthFactor;
	mulFac = !mulFac ? 0.0f : 1.0f / mulFac;

	float overFac[4] = { flDepthFactor, mulFac, m_Key.AlphaTestReference, 0.0f };

	AfxD3D9_OverrideBegin_ps_c31(overFac);

	// Bind normal material:
	return material;
}

void CAfxBaseFxStream::CActionAfxSpritecardHook::SetVertexShader(CAfx_csgo_ShaderState & state)
{
	char const * shaderName = state.Static.SetVertexShader.pFileName.c_str();

	if(!strcmp(shaderName,"splinecard_vs20"))
	{
		int remainder = m_Combos_splinecard_vs20.CalcCombos(state.Static.SetVertexShader.nStaticVshIndex, state.Dynamic.SetVertexShaderIndex.vshIndex);

		int combo = ShaderCombo_afxHook_splinecard_vs20::GetCombo(
			(ShaderCombo_afxHook_splinecard_vs20::ORIENTATION_e)m_Combos_splinecard_vs20.m_Orientation,
			(ShaderCombo_afxHook_splinecard_vs20::ADDBASETEXTURE2_e)m_Combos_splinecard_vs20.m_ADDBASETEXTURE2,
			(ShaderCombo_afxHook_splinecard_vs20::EXTRACTGREENALPHA_e)m_Combos_splinecard_vs20.m_EXTRACTGREENALPHA,
			(ShaderCombo_afxHook_splinecard_vs20::DUALSEQUENCE_e)m_Combos_splinecard_vs20.m_DUALSEQUENCE,
			(ShaderCombo_afxHook_splinecard_vs20::DEPTHBLEND_e)m_Combos_splinecard_vs20.m_DEPTHBLEND,
			(ShaderCombo_afxHook_splinecard_vs20::PACKED_INTERPOLATOR_e)m_Combos_splinecard_vs20.m_PACKED_INTERPOLATOR,
			(ShaderCombo_afxHook_splinecard_vs20::ANIMBLEND_OR_MAXLUMFRAMEBLEND1_e)m_Combos_splinecard_vs20.m_ANIMBLEND_OR_MAXLUMFRAMEBLEND1		
		);

		IAfxVertexShader * afxVertexShader = g_AfxShaders.GetAcsVertexShader("afxHook_splinecard_vs20.acs", combo);

		if(!afxVertexShader->GetVertexShader())
			Tier0_Warning("AFXERROR: CAfxBaseFxStream::CActionAfxSpritecardHook::SetVertexShader: Replacement Shader combo %i for %s is null.\n", combo, shaderName);
		else
		{
			// Override shader:
			AfxD3D9_OverrideBegin_SetVertexShader(afxVertexShader->GetVertexShader());
		}

		afxVertexShader->Release();

		return;
	}
	else
	if(!strcmp(shaderName,"spritecard_vs20"))
	{
		int remainder = m_Combos_spritecard_vs20.CalcCombos(state.Static.SetVertexShader.nStaticVshIndex, state.Dynamic.SetVertexShaderIndex.vshIndex);

		int combo = ShaderCombo_afxHook_spritecard_vs20::GetCombo(
			(ShaderCombo_afxHook_spritecard_vs20::ORIENTATION_e)m_Combos_spritecard_vs20.m_Orientation,
			(ShaderCombo_afxHook_spritecard_vs20::ZOOM_ANIMATE_SEQ2_e)m_Combos_spritecard_vs20.m_ZOOM_ANIMATE_SEQ2,
			(ShaderCombo_afxHook_spritecard_vs20::DUALSEQUENCE_e)m_Combos_spritecard_vs20.m_DUALSEQUENCE,
			(ShaderCombo_afxHook_spritecard_vs20::ADDBASETEXTURE2_e)m_Combos_spritecard_vs20.m_ADDBASETEXTURE2,
			(ShaderCombo_afxHook_spritecard_vs20::EXTRACTGREENALPHA_e)m_Combos_spritecard_vs20.m_EXTRACTGREENALPHA,
			(ShaderCombo_afxHook_spritecard_vs20::DEPTHBLEND_e)m_Combos_spritecard_vs20.m_DEPTHBLEND,
			(ShaderCombo_afxHook_spritecard_vs20::ANIMBLEND_OR_MAXLUMFRAMEBLEND1_e)m_Combos_spritecard_vs20.m_ANIMBLEND_OR_MAXLUMFRAMEBLEND1,
			(ShaderCombo_afxHook_spritecard_vs20::CROP_e)m_Combos_spritecard_vs20.m_CROP,
			(ShaderCombo_afxHook_spritecard_vs20::PACKED_INTERPOLATOR_e)m_Combos_spritecard_vs20.m_PACKED_INTERPOLATOR,
			(ShaderCombo_afxHook_spritecard_vs20::SPRITECARDVERTEXFOG_e)m_Combos_spritecard_vs20.m_SPRITECARDVERTEXFOG,
			(ShaderCombo_afxHook_spritecard_vs20::HARDWAREFOGBLEND_e)m_Combos_spritecard_vs20.m_HARDWAREFOGBLEND,
			(ShaderCombo_afxHook_spritecard_vs20::PERPARTICLEOUTLINE_e)m_Combos_spritecard_vs20.m_PERPARTICLEOUTLINE
		);

		IAfxVertexShader * afxVertexShader = g_AfxShaders.GetAcsVertexShader("afxHook_spritecard_vs20.acs", combo);

		if(!afxVertexShader->GetVertexShader())
			Tier0_Warning("AFXERROR: CAfxBaseFxStream::CActionAfxSpritecardHook::SetVertexShader: Replacement Shader combo %i for %s is null.\n", combo, shaderName);
		else
		{
			// Override shader:
			AfxD3D9_OverrideBegin_SetVertexShader(afxVertexShader->GetVertexShader());
		}

		afxVertexShader->Release();

		return;
	}
	else
		Tier0_Warning("AFXERROR: CAfxBaseFxStream::CActionAfxSpritecardHook::SetVertexShader: No replacement defined for %s.\n", shaderName);
}

void CAfxBaseFxStream::CActionAfxSpritecardHook::SetPixelShader(CAfx_csgo_ShaderState & state)
{
	char const * shaderName = state.Static.SetPixelShader.pFileName.c_str();

	if(!strcmp(shaderName,"spritecard_ps20"))
	{
		static bool firstPass = true;
		if(firstPass)
		{
			firstPass = false;
			Tier0_Warning("AFXWARNING: You are using an untested code path in CAfxBaseFxStream::CActionAfxSpritecardHook::SetPixelShader for %s.\n", shaderName);
		}

		ShaderCombo_afxHook_spritecard_ps20::AFXORGBLENDMODE_e afxOrgBlendMode;

		if(state.Static.EnableBlending.bEnable)
		{
			if(SHADER_BLEND_DST_COLOR == state.Static.BlendFunc.srcFactor
				&& SHADER_BLEND_SRC_COLOR == state.Static.BlendFunc.dstFactor)
			{
				afxOrgBlendMode = ShaderCombo_afxHook_spritecard_ps20::AFXORGBLENDMODE_0;
			}
			else
			if(SHADER_BLEND_ONE == state.Static.BlendFunc.srcFactor
				&& SHADER_BLEND_ONE_MINUS_SRC_ALPHA == state.Static.BlendFunc.dstFactor)
			{
				afxOrgBlendMode = ShaderCombo_afxHook_spritecard_ps20::AFXORGBLENDMODE_1;
			}
			else
			if(SHADER_BLEND_SRC_ALPHA == state.Static.BlendFunc.srcFactor
				&& SHADER_BLEND_ONE == state.Static.BlendFunc.dstFactor)
			{
				afxOrgBlendMode = ShaderCombo_afxHook_spritecard_ps20::AFXORGBLENDMODE_2;
			}
			else
			if(SHADER_BLEND_SRC_ALPHA == state.Static.BlendFunc.srcFactor
				&& SHADER_BLEND_ONE_MINUS_SRC_ALPHA == state.Static.BlendFunc.dstFactor)
			{
				afxOrgBlendMode = ShaderCombo_afxHook_spritecard_ps20::AFXORGBLENDMODE_3;
			}
			else
			{
				Tier0_Warning("AFXERROR: CAfxBaseFxStream::CActionAfxSpritecardHook::SetPixelShader: current blend mode not supported for %s.\n", shaderName);
				return;
			}
		}
		else
		{
			Tier0_Warning("AFXERROR: CAfxBaseFxStream::CActionAfxSpritecardHook::SetPixelShader: non-blending mode not supported for %s.\n", shaderName);
			return;
		}

		int remainder = m_Combos_ps20.CalcCombos(state.Static.SetPixelShader.nStaticPshIndex, state.Dynamic.SetPixelShaderIndex.pshIndex);

		int combo = ShaderCombo_afxHook_spritecard_ps20::GetCombo(
			(ShaderCombo_afxHook_spritecard_ps20::AFXMODE_e)m_Key.AFXMODE,
			afxOrgBlendMode,
			(ShaderCombo_afxHook_spritecard_ps20::DUALSEQUENCE_e)m_Combos_ps20.m_DUALSEQUENCE,
			(ShaderCombo_afxHook_spritecard_ps20::SEQUENCE_BLEND_MODE_e)m_Combos_ps20.m_SEQUENCE_BLEND_MODE,
			(ShaderCombo_afxHook_spritecard_ps20::ADDBASETEXTURE2_e)m_Combos_ps20.m_ADDBASETEXTURE2,
			(ShaderCombo_afxHook_spritecard_ps20::MAXLUMFRAMEBLEND1_e)m_Combos_ps20.m_MAXLUMFRAMEBLEND1,
			(ShaderCombo_afxHook_spritecard_ps20::MAXLUMFRAMEBLEND2_e)m_Combos_ps20.m_MAXLUMFRAMEBLEND2,
			(ShaderCombo_afxHook_spritecard_ps20::EXTRACTGREENALPHA_e)m_Combos_ps20.m_EXTRACTGREENALPHA,
			(ShaderCombo_afxHook_spritecard_ps20::COLORRAMP_e)m_Combos_ps20.m_COLORRAMP,
			(ShaderCombo_afxHook_spritecard_ps20::ANIMBLEND_e)m_Combos_ps20.m_ANIMBLEND,
			(ShaderCombo_afxHook_spritecard_ps20::ADDSELF_e)m_Combos_ps20.m_ADDSELF,
			(ShaderCombo_afxHook_spritecard_ps20::MOD2X_e)m_Combos_ps20.m_MOD2X,
			(ShaderCombo_afxHook_spritecard_ps20::COLOR_LERP_PS_e)m_Combos_ps20.m_COLOR_LERP_PS,
			(ShaderCombo_afxHook_spritecard_ps20::PACKED_INTERPOLATOR_e)m_Combos_ps20.m_PACKED_INTERPOLATOR,
			(ShaderCombo_afxHook_spritecard_ps20::DISTANCEALPHA_e)m_Combos_ps20.m_DISTANCEALPHA,
			(ShaderCombo_afxHook_spritecard_ps20::SOFTEDGES_e)m_Combos_ps20.m_SOFTEDGES,
			(ShaderCombo_afxHook_spritecard_ps20::OUTLINE_e)m_Combos_ps20.m_OUTLINE,
			(ShaderCombo_afxHook_spritecard_ps20::MULOUTPUTBYALPHA_e)m_Combos_ps20.m_MULOUTPUTBYALPHA
		);

		IAfxPixelShader * afxPixelShader = g_AfxShaders.GetAcsPixelShader("afxHook_spritecard_ps20.acs", combo);

		if(!afxPixelShader->GetPixelShader())
			Tier0_Warning("AFXERROR: CAfxBaseFxStream::CActionAfxSpritecardHook::SetPixelShader: Replacement Shader combo %i for %s is null.\n", combo, shaderName);
		else
		{
			// Override shader:
			AfxD3D9_OverrideBegin_SetPixelShader(afxPixelShader->GetPixelShader());
		}

		afxPixelShader->Release();

		return;
	}
	else
	if(!strcmp(shaderName,"spritecard_ps20b"))
	{
		ShaderCombo_afxHook_spritecard_ps20b::AFXORGBLENDMODE_e afxOrgBlendMode;

		if(state.Static.EnableBlending.bEnable)
		{
			if(SHADER_BLEND_DST_COLOR == state.Static.BlendFunc.srcFactor
				&& SHADER_BLEND_SRC_COLOR == state.Static.BlendFunc.dstFactor)
			{
				afxOrgBlendMode = ShaderCombo_afxHook_spritecard_ps20b::AFXORGBLENDMODE_0;
			}
			else
			if(SHADER_BLEND_ONE == state.Static.BlendFunc.srcFactor
				&& SHADER_BLEND_ONE_MINUS_SRC_ALPHA == state.Static.BlendFunc.dstFactor)
			{
				afxOrgBlendMode = ShaderCombo_afxHook_spritecard_ps20b::AFXORGBLENDMODE_1;
			}
			else
			if(SHADER_BLEND_SRC_ALPHA == state.Static.BlendFunc.srcFactor
				&& SHADER_BLEND_ONE == state.Static.BlendFunc.dstFactor)
			{
				afxOrgBlendMode = ShaderCombo_afxHook_spritecard_ps20b::AFXORGBLENDMODE_2;
			}
			else
			if(SHADER_BLEND_SRC_ALPHA == state.Static.BlendFunc.srcFactor
				&& SHADER_BLEND_ONE_MINUS_SRC_ALPHA == state.Static.BlendFunc.dstFactor)
			{
				afxOrgBlendMode = ShaderCombo_afxHook_spritecard_ps20b::AFXORGBLENDMODE_3;
			}
			else
			{
				Tier0_Warning("AFXERROR: CAfxBaseFxStream::CActionAfxSpritecardHook::SetPixelShader: current blend mode not supported for %s.\n", shaderName);
				return;
			}
		}
		else
		{
			Tier0_Warning("AFXERROR: CAfxBaseFxStream::CActionAfxSpritecardHook::SetPixelShader: non-blending mode not supported for %s.\n", shaderName);
			return;
		}

		int remainder = m_Combos_ps20b.CalcCombos(state.Static.SetPixelShader.nStaticPshIndex, state.Dynamic.SetPixelShaderIndex.pshIndex);

		int combo = ShaderCombo_afxHook_spritecard_ps20b::GetCombo(
			(ShaderCombo_afxHook_spritecard_ps20b::AFXMODE_e)m_Key.AFXMODE,
			afxOrgBlendMode,
			(ShaderCombo_afxHook_spritecard_ps20b::DUALSEQUENCE_e)m_Combos_ps20b.m_DUALSEQUENCE,
			(ShaderCombo_afxHook_spritecard_ps20b::SEQUENCE_BLEND_MODE_e)m_Combos_ps20b.m_SEQUENCE_BLEND_MODE,
			(ShaderCombo_afxHook_spritecard_ps20b::ADDBASETEXTURE2_e)m_Combos_ps20b.m_ADDBASETEXTURE2,
			(ShaderCombo_afxHook_spritecard_ps20b::MAXLUMFRAMEBLEND1_e)m_Combos_ps20b.m_MAXLUMFRAMEBLEND1,
			(ShaderCombo_afxHook_spritecard_ps20b::MAXLUMFRAMEBLEND2_e)m_Combos_ps20b.m_MAXLUMFRAMEBLEND2,
			(ShaderCombo_afxHook_spritecard_ps20b::EXTRACTGREENALPHA_e)m_Combos_ps20b.m_EXTRACTGREENALPHA,
			(ShaderCombo_afxHook_spritecard_ps20b::COLORRAMP_e)m_Combos_ps20b.m_COLORRAMP,
			(ShaderCombo_afxHook_spritecard_ps20b::ANIMBLEND_e)m_Combos_ps20b.m_ANIMBLEND,
			(ShaderCombo_afxHook_spritecard_ps20b::ADDSELF_e)m_Combos_ps20b.m_ADDSELF,
			(ShaderCombo_afxHook_spritecard_ps20b::MOD2X_e)m_Combos_ps20b.m_MOD2X,
			(ShaderCombo_afxHook_spritecard_ps20b::COLOR_LERP_PS_e)m_Combos_ps20b.m_COLOR_LERP_PS,
			(ShaderCombo_afxHook_spritecard_ps20b::PACKED_INTERPOLATOR_e)m_Combos_ps20b.m_PACKED_INTERPOLATOR,
			(ShaderCombo_afxHook_spritecard_ps20b::DISTANCEALPHA_e)m_Combos_ps20b.m_DISTANCEALPHA,
			(ShaderCombo_afxHook_spritecard_ps20b::SOFTEDGES_e)m_Combos_ps20b.m_SOFTEDGES,
			(ShaderCombo_afxHook_spritecard_ps20b::OUTLINE_e)m_Combos_ps20b.m_OUTLINE,
			(ShaderCombo_afxHook_spritecard_ps20b::MULOUTPUTBYALPHA_e)m_Combos_ps20b.m_MULOUTPUTBYALPHA
		);

		/*
		Tier0_Msg(
			"%i -> %i %i | %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i (%i)\n",
			combo,
			(ShaderCombo_afxHook_spritecard_ps20b::AFXMODE_e)m_Key.AFXMODE,
			afxOrgBlendMode,
			(ShaderCombo_afxHook_spritecard_ps20b::DUALSEQUENCE_e)m_Combos_ps20b.m_DUALSEQUENCE,
			(ShaderCombo_afxHook_spritecard_ps20b::SEQUENCE_BLEND_MODE_e)m_Combos_ps20b.m_SEQUENCE_BLEND_MODE,
			(ShaderCombo_afxHook_spritecard_ps20b::ADDBASETEXTURE2_e)m_Combos_ps20b.m_ADDBASETEXTURE2,
			(ShaderCombo_afxHook_spritecard_ps20b::MAXLUMFRAMEBLEND1_e)m_Combos_ps20b.m_MAXLUMFRAMEBLEND1,
			(ShaderCombo_afxHook_spritecard_ps20b::MAXLUMFRAMEBLEND2_e)m_Combos_ps20b.m_MAXLUMFRAMEBLEND2,
			(ShaderCombo_afxHook_spritecard_ps20b::EXTRACTGREENALPHA_e)m_Combos_ps20b.m_EXTRACTGREENALPHA,
			(ShaderCombo_afxHook_spritecard_ps20b::COLORRAMP_e)m_Combos_ps20b.m_COLORRAMP,
			(ShaderCombo_afxHook_spritecard_ps20b::ANIMBLEND_e)m_Combos_ps20b.m_ANIMBLEND,
			(ShaderCombo_afxHook_spritecard_ps20b::ADDSELF_e)m_Combos_ps20b.m_ADDSELF,
			(ShaderCombo_afxHook_spritecard_ps20b::MOD2X_e)m_Combos_ps20b.m_MOD2X,
			(ShaderCombo_afxHook_spritecard_ps20b::COLOR_LERP_PS_e)m_Combos_ps20b.m_COLOR_LERP_PS,
			(ShaderCombo_afxHook_spritecard_ps20b::PACKED_INTERPOLATOR_e)m_Combos_ps20b.m_PACKED_INTERPOLATOR,
			(ShaderCombo_afxHook_spritecard_ps20b::DISTANCEALPHA_e)m_Combos_ps20b.m_DISTANCEALPHA,
			(ShaderCombo_afxHook_spritecard_ps20b::SOFTEDGES_e)m_Combos_ps20b.m_SOFTEDGES,
			(ShaderCombo_afxHook_spritecard_ps20b::OUTLINE_e)m_Combos_ps20b.m_OUTLINE,
			(ShaderCombo_afxHook_spritecard_ps20b::MULOUTPUTBYALPHA_e)m_Combos_ps20b.m_MULOUTPUTBYALPHA,
			remainder
			);
		*/

		IAfxPixelShader * afxPixelShader = g_AfxShaders.GetAcsPixelShader("afxHook_spritecard_ps20b.acs", combo);

		if(!afxPixelShader->GetPixelShader())
			Tier0_Warning("AFXERROR: CAfxBaseFxStream::CActionAfxSpritecardHook::SetPixelShader: Replacement Shader combo %i for %s is null.\n", combo, shaderName);
		else
		{
			// Override shader:
			AfxD3D9_OverrideBegin_SetPixelShader(afxPixelShader->GetPixelShader());
		}

		afxPixelShader->Release();
	}
	else
		Tier0_Warning("AFXERROR: CAfxBaseFxStream::CActionAfxSpritecardHook::SetPixelShader: No replacement defined for %s.\n", shaderName);

}

// CAfxBaseFxStream::CActionUnlitGenericFallback ///////////////////////////////

CAfxBaseFxStream::CActionUnlitGenericFallback::CActionUnlitGenericFallback(CActionAfxVertexLitGenericHookKey & key, char const * unlitGenericFallbackMaterialName)
: CActionAfxVertexLitGenericHook(key)
, m_Material(0)
, m_MaterialName(unlitGenericFallbackMaterialName)
{
}

CAfxBaseFxStream::CActionUnlitGenericFallback::~CActionUnlitGenericFallback()
{
	if(m_Material) delete m_Material;
}

IMaterial_csgo * CAfxBaseFxStream::CActionUnlitGenericFallback::MaterialHook(IAfxMatRenderContext * ctx, IMaterial_csgo * material)
{
	if(!m_Material) m_Material = new CAfxMaterial(
		CAfxBaseFxStream::m_Shared.m_ActiveBaseFxStream->m_Streams->GetFreeMaster(),
		CAfxBaseFxStream::m_Shared.m_ActiveBaseFxStream->m_Streams->GetMaterialSystem()->FindMaterial(m_MaterialName.c_str(), 0));

	return CActionAfxVertexLitGenericHook::MaterialHook(ctx, m_Material->GetMaterial());
}

// CAfxBaseFxStream::CActionAfxSplineRopeHook //////////////////////////////////

csgo_Stdshader_dx9_Combos_splinerope_ps20 CAfxBaseFxStream::CActionAfxSplineRopeHook::m_Combos_ps20;
csgo_Stdshader_dx9_Combos_splinerope_ps20b CAfxBaseFxStream::CActionAfxSplineRopeHook::m_Combos_ps20b;

CAfxBaseFxStream::CActionAfxSplineRopeHook::CActionAfxSplineRopeHook(CActionAfxSplineRopeHookKey & key)
: CAction()
, m_Key(key)
{
}

void CAfxBaseFxStream::CActionAfxSplineRopeHook::AfxUnbind(IAfxMatRenderContext * ctx)
{
	AfxD3D9_OverrideEnd_ps_c31();

	AfxD3D9OverrideEnd_D3DRS_SRGBWRITEENABLE();
	
	//AfxD3D9OverrideEnd_D3DRS_MULTISAMPLEANTIALIAS();

	AfxD3D9_OverrideEnd_SetPixelShader();
}

IMaterial_csgo * CAfxBaseFxStream::CActionAfxSplineRopeHook::MaterialHook(IAfxMatRenderContext * ctx, IMaterial_csgo * material)
{
	// depth factors:

	float scale = g_bIn_csgo_CSkyBoxView_Draw ? csgo_CSkyBoxView_GetScale() : 1.0f;
	float flDepthFactor = scale * CAfxBaseFxStream::m_Shared.m_ActiveBaseFxStream->m_DepthVal;
	float flDepthFactorMax = scale * CAfxBaseFxStream::m_Shared.m_ActiveBaseFxStream->m_DepthValMax;

	// Foce multisampling off for depth24:
	//if(m_Key.AFXMODE == ShaderCombo_afxHook_splinerope_ps20b::AFXMODE_1)
	//	AfxD3D9OverrideBegin_D3DRS_MULTISAMPLEANTIALIAS(FALSE);

	// Force SRGBWriteEnable to off:
	AfxD3D9OverrideBegin_D3DRS_SRGBWRITEENABLE(FALSE);

	// Fill in g_AfxConstants in shader:

	float mulFac = flDepthFactorMax -flDepthFactor;
	mulFac = !mulFac ? 0.0f : 1.0f / mulFac;

	float overFac[4] = { flDepthFactor, mulFac, m_Key.AlphaTestReference, 0.0f };

	AfxD3D9_OverrideBegin_ps_c31(overFac);

	// Bind normal material:
	return material;
	//return m_Material.GetMaterial();
}

void CAfxBaseFxStream::CActionAfxSplineRopeHook::SetPixelShader(CAfx_csgo_ShaderState & state)
{
	char const * shaderName = state.Static.SetPixelShader.pFileName.c_str();

	if(!strcmp(shaderName,"splinerope_ps20"))
	{
		static bool firstPass = true;
		if(firstPass)
		{
			firstPass = false;
			Tier0_Warning("AFXWARNING: You are using an untested code path in CAfxBaseFxStream::CActionAfxSplineRopeHook::SetPixelShader for %s.\n", shaderName);
		}

		m_Combos_ps20.CalcCombos(state.Static.SetPixelShader.nStaticPshIndex, state.Dynamic.SetPixelShaderIndex.pshIndex);

		if(0 < m_Combos_ps20.m_SHADOWDEPTH)
		{
			Tier0_Warning("AFXERROR: CAfxBaseFxStream::CActionAfxSplineRopeHook::SetPixelShader: 0 < m_SHADOWDEPTH not supported for %s.\n", shaderName);
			return;
		}

		int combo = ShaderCombo_afxHook_splinerope_ps20::GetCombo(
			(ShaderCombo_afxHook_splinerope_ps20::AFXMODE_e)m_Key.AFXMODE,
			(ShaderCombo_afxHook_splinerope_ps20::SHADER_SRGB_READ_e)m_Combos_ps20.m_SHADER_SRGB_READ,
			(ShaderCombo_afxHook_splinerope_ps20::ALPHATESTREF_e)m_Combos_ps20.m_ALPHATESTREF
		);

		IAfxPixelShader * afxPixelShader = g_AfxShaders.GetAcsPixelShader("afxHook_splinerope_ps20.acs", combo);

		if(!afxPixelShader->GetPixelShader())
			Tier0_Warning("AFXERROR: CAfxBaseFxStream::CActionAfxSplineRopeHook::SetPixelShader: Replacement Shader combo %i for %s is null.\n", combo, shaderName);
		else
		{
			// Override shader:
			AfxD3D9_OverrideBegin_SetPixelShader(afxPixelShader->GetPixelShader());
		}

		afxPixelShader->Release();
	}
	else
	if(!strcmp(shaderName,"splinerope_ps20b"))
	{
		m_Combos_ps20b.CalcCombos(state.Static.SetPixelShader.nStaticPshIndex, state.Dynamic.SetPixelShaderIndex.pshIndex);

		/*
		static bool firstPass = true;
		if(firstPass)
		{
			firstPass = false;
			
			Tier0_Warning("Requesting dump of shader %s %i_%i_%i_%i_%i.\n", shaderName,
				m_Combos_ps20b.m_WRITE_DEPTH_TO_DESTALPHA,
				m_Combos_ps20b.m_PIXELFOGTYPE,
				m_Combos_ps20b.m_SHADER_SRGB_READ,
				m_Combos_ps20b.m_SHADOWDEPTH,
				m_Combos_ps20b.m_ALPHATESTREF
				);
			
			g_bD3D9DumpPixelShader = true;
		}

		return;
		*/

		if(0 < m_Combos_ps20b.m_SHADOWDEPTH)
		{
			Tier0_Warning("AFXERROR: CAfxBaseFxStream::CActionAfxSplineRopeHook::SetPixelShader: 0 < m_SHADOWDEPTH not supported for %s.\n", shaderName);
			return;
		}

		int combo = ShaderCombo_afxHook_splinerope_ps20b::GetCombo(
			(ShaderCombo_afxHook_splinerope_ps20b::AFXMODE_e)m_Key.AFXMODE,
			(ShaderCombo_afxHook_splinerope_ps20b::SHADER_SRGB_READ_e)m_Combos_ps20b.m_SHADER_SRGB_READ,
			(ShaderCombo_afxHook_splinerope_ps20b::ALPHATESTREF_e)m_Combos_ps20b.m_ALPHATESTREF
		);

		IAfxPixelShader * afxPixelShader = g_AfxShaders.GetAcsPixelShader("afxHook_splinerope_ps20b.acs", combo);

		if(!afxPixelShader->GetPixelShader())
			Tier0_Warning("AFXERROR: CAfxBaseFxStream::CActionAfxSplineRopeHook::SetPixelShader: Replacement Shader combo %i for %s is null.\n", combo, shaderName);
		else
		{
			// Override shader:
			AfxD3D9_OverrideBegin_SetPixelShader(afxPixelShader->GetPixelShader());
		}

		afxPixelShader->Release();
	}
	else
		Tier0_Warning("AFXERROR: CAfxBaseFxStream::CActionAfxSplineRopeHook::SetPixelShader: No replacement defined for %s.\n", shaderName);

}
#endif

// CAfxStreams /////////////////////////////////////////////////////////////////

void FinishStreamForGlowOverlayFix(CAfxRecordStream * stream)
{
	if (CAfxSingleStream * singleStream = stream->AsAfxSingleStream())
	{
		//GetCsgoCGlowOverlayFix()->OnStreamFinished(singleStream->Stream_get());
	}
	if (CAfxTwinStream * twinStream = stream->AsAfxTwinStream())
	{
		//GetCsgoCGlowOverlayFix()->OnStreamFinished(twinStream->StreamA_get());
		//GetCsgoCGlowOverlayFix()->OnStreamFinished(twinStream->StreamB_get());
	}
}

CAfxStreams::CAfxStreams()
: m_RecordName("untitled_rec")
, m_PresentRecordOnScreen(false)
, m_StartMovieWav(true)
, m_MaterialSystem(0)
, m_AfxBaseClientDll(0)
, m_ShaderShadow(0)
, m_PreviewStream(0)
, m_Recording(false)
, m_Frame(0)
, m_MatPostProcessEnableRef(0)
, m_MatDynamicTonemappingRef(0)
, m_MatMotionBlurEnabledRef(0)
, m_MatForceTonemapScale(0)
, m_FormatBmpAndNotTga(false)
, m_Current_View_Render_ThreadId(0)
//, m_RgbaRenderTarget(0)
, m_RenderTargetDepthF(0)
, m_CamBvh(false)
, m_HostFrameRate(0)
, m_GameRecording(false)
{
}

CAfxStreams::~CAfxStreams()
{
	ShutDown();
}


void CAfxStreams::OnMaterialSystem(SOURCESDK::IMaterialSystem_csgo * value)
{
	m_MaterialSystem = value;

	CreateRenderTargets(value);
}

void CAfxStreams::SetCurrent_View_Render_ThreadId(DWORD id)
{
	InterlockedExchange(&m_Current_View_Render_ThreadId, id);
}

DWORD CAfxStreams::GetCurrent_View_Render_ThreadId()
{
	return InterlockedCompareExchange(&m_Current_View_Render_ThreadId, -1, -1);
}


void CAfxStreams::OnAfxBaseClientDll(IAfxBaseClientDll * value)
{
	m_AfxBaseClientDll = value;
	if(m_AfxBaseClientDll)
	{
		m_AfxBaseClientDll->OnLevelShutdown_set(this);
		m_AfxBaseClientDll->OnView_Render_set(this);
	}
}

void CAfxStreams::OnAfxBaseClientDll_Free(void)
{
	if(m_RenderTargetDepthF)
	{
		m_RenderTargetDepthF->DecrementReferenceCount();
		m_RenderTargetDepthF = 0;
	}
	/*
	if(m_RgbaRenderTarget)
	{
		m_RgbaRenderTarget->DecrementReferenceCount();
		m_RgbaRenderTarget = 0;
	}
	*/

	if(m_AfxBaseClientDll)
	{
		m_AfxBaseClientDll->OnView_Render_set(0);
		m_AfxBaseClientDll->OnLevelShutdown_set(0);
		m_AfxBaseClientDll = 0;
	}
}

void CAfxStreams::OnShaderShadow(SOURCESDK::IShaderShadow_csgo * value)
{
	m_ShaderShadow = value;
}

#if AFX_SHADERS_CSGO
void CAfxStreams::OnSetVertexShader(CAfx_csgo_ShaderState & state)
{
	IAfxContextHook * hook = FindHook(GetCurrentContext());

	if (hook)
		hook->SetVertexShader(state);
}

void CAfxStreams::OnSetPixelShader(CAfx_csgo_ShaderState & state)
{
	IAfxContextHook * hook = FindHook(GetCurrentContext());

	if (hook)
		hook->SetPixelShader(state);
}
#endif

float CAfxStreams::OnRenderSmokeOverlayAlphaMod(void)
{
	IAfxContextHook * hook = FindHook(GetCurrentContext());

	if (hook)
		return hook->RenderSmokeOverlayAlphaMod();

	return 1.0f;
}

bool CAfxStreams::OnViewRenderShouldForceNoVis(bool orgValue)
{
	IAfxContextHook * hook = FindHook(GetCurrentContext());

	if (hook)
		return hook->ViewRenderShouldForceNoVis(orgValue);

	return orgValue;
}

void CAfxStreams::OnDrawingHud(void)
{
	IAfxContextHook * hook = FindHook(GetCurrentContext());

	if (hook)
		hook->DrawingHudBegin();
}

void CAfxStreams::OnDrawingSkyBoxViewBegin(void)
{
	IAfxContextHook * hook = FindHook(GetCurrentContext());

	if (hook)
		hook->DrawingSkyBoxViewBegin();
}

void CAfxStreams::OnDrawingSkyBoxViewEnd(void)
{
	IAfxContextHook * hook = FindHook(GetCurrentContext());

	if (hook)
		hook->DrawingSkyBoxViewEnd();
}


void CAfxStreams::Console_RecordName_set(const char * value)
{
	m_RecordName.assign(value);
}

const char * CAfxStreams::Console_RecordName_get()
{
	return m_RecordName.c_str();
}

void CAfxStreams::Console_PresentRecordOnScreen_set(bool value)
{
	m_PresentRecordOnScreen = value;
}

bool CAfxStreams::Console_PresentRecordOnScreen_get()
{
	return m_PresentRecordOnScreen;
}

void CAfxStreams::Console_StartMovieWav_set(bool value)
{
	m_StartMovieWav = value;
}

bool CAfxStreams::Console_StartMovieWav_get()
{
	return m_StartMovieWav;
}

void CAfxStreams::Console_MatPostprocessEnable_set(int value)
{
	m_NewMatPostProcessEnable = value;
}

int CAfxStreams::Console_MatPostprocessEnable_get()
{
	return m_NewMatPostProcessEnable;
}

void CAfxStreams::Console_MatDynamicToneMapping_set(int value)
{
	m_NewMatDynamicTonemapping = value;
}

int CAfxStreams::Console_MatDynamicToneMapping_get()
{
	return m_NewMatDynamicTonemapping;
}

void CAfxStreams::Console_MatMotionBlurEnabled_set(int value)
{
	m_NewMatMotionBlurEnabled = value;
}

int CAfxStreams::Console_MatMotionBlurEnabled_get()
{
	return m_NewMatMotionBlurEnabled;
}

void CAfxStreams::Console_MatForceTonemapScale_set(float value)
{
	m_NewMatForceTonemapScale = value;
}

float CAfxStreams::Console_MatForceTonemapScale_get()
{
	return m_NewMatForceTonemapScale;
}

void CAfxStreams::Console_RecordFormat_set(const char * value)
{
	if(!_stricmp(value, "bmp"))
		m_FormatBmpAndNotTga = true;
	else
	if(!_stricmp(value, "tga"))
		m_FormatBmpAndNotTga = false;
	else
		Tier0_Warning("Error: Invalid format %s\n.", value);
}

const char * CAfxStreams::Console_RecordFormat_get()
{
	return m_FormatBmpAndNotTga ? "bmp" : "tga";
}

void CAfxStreams::Console_Record_Start()
{
	Console_Record_End();

	Tier0_Msg("Starting recording ... ");
	
	if(AnsiStringToWideString(m_RecordName.c_str(), m_TakeDir)
		&& (m_TakeDir.append(L"\\take"), SuggestTakePath(m_TakeDir.c_str(), 4, m_TakeDir))
		&& CreatePath(m_TakeDir.c_str(), m_TakeDir)
	)
	{
		m_Recording = true;
		m_Frame = 0;
		m_StartMovieWavUsed = false;

		std::string ansiTakeDir;
		bool ansiTakeDirOk = WideStringToAnsiString(m_TakeDir.c_str(), ansiTakeDir);

		BackUpMatVars();
		SetMatVarsForStreams();

		if (!m_HostFrameRate)
			m_HostFrameRate = new WrpConVarRef("host_framerate");

		double frameTime = m_HostFrameRate->GetFloat();
		if (1.0 <= frameTime) frameTime = 1.0 / frameTime;

		for(std::list<CAfxRecordStream *>::iterator it = m_Streams.begin(); it != m_Streams.end(); ++it)
		{
			(*it)->RecordStart();
		}

		if (m_GameRecording)
		{
			std::wstring fileName(m_TakeDir);
			fileName.append(L"\\afxGameRecord.agr");

			if(CClientTools * instance = CClientTools::Instance()) instance->StartRecording(fileName.c_str());
		}

		if (m_CamBvh)
		{
			std::wstring camFileName(m_TakeDir);
			camFileName.append(L"\\cam_main.bvh");

			g_Hook_VClient_RenderView.ExportBegin(camFileName.c_str(), frameTime);
		}

		for (std::list<CEntityBvhCapture *>::iterator it = m_EntityBvhCaptures.begin(); it != m_EntityBvhCaptures.end(); ++it)
		{
			(*it)->StartCapture(m_TakeDir, frameTime);
		}

		if (m_CamExport && ansiTakeDirOk)
		{
			std::string camFileName(ansiTakeDir);
			camFileName.append("\\cam_main.cam");

			m_CamExportObj = new CamExport(camFileName.c_str(), m_CamExportScaleFov);
		}

		Tier0_Msg("done.\n");

		Tier0_Msg("Recording to \"%s\".\n", ansiTakeDirOk ? ansiTakeDir.c_str() : "?");

		if (ansiTakeDirOk)
		{
			m_StartMovieWavUsed = m_StartMovieWav;

			if (m_StartMovieWavUsed)
			{
				if (!csgo_Audio_StartRecording(ansiTakeDir.c_str()))
					Tier0_Warning("Error: Could not start WAV audio recording!\n");
			}
		}
		else
			Tier0_Warning("Error: Cannot convert take directory to ansi string. I.e. sound recording might be affected!\n");
	}
	else
	{
		Tier0_Msg("FAILED");
		Tier0_Warning("Error: Failed to create directories for \"%s\".\n", m_RecordName.c_str());
	}
}

void CAfxStreams::Console_Record_End()
{
	if(m_Recording)
	{
		Tier0_Msg("Finishing recording ... ");

		if (m_StartMovieWavUsed)
		{
			csgo_Audio_EndRecording();
		}

		if (m_CamExportObj)
		{
			delete m_CamExportObj;
			m_CamExportObj = 0;
		}

		if (m_CamBvh)
		{
			g_Hook_VClient_RenderView.ExportEnd();
		}

		for (std::list<CEntityBvhCapture *>::iterator it = m_EntityBvhCaptures.begin(); it != m_EntityBvhCaptures.end(); ++it)
		{
			(*it)->EndCapture();
		}

		if (m_GameRecording)
		{
			if (CClientTools * instance = CClientTools::Instance()) instance->EndRecording();
		}

		for(std::list<CAfxRecordStream *>::iterator it = m_Streams.begin(); it != m_Streams.end(); ++it)
		{
			(*it)->RecordEnd();

			FinishStreamForGlowOverlayFix(*it);
		}

		RestoreMatVars();

		Tier0_Msg("done.\n");

		//AfxD3D9_Block_Present(false);
	}

	m_Recording = false;
}

void CAfxStreams::Console_AddStream(const char * streamName)
{
	if(!Console_CheckStreamName(streamName))
		return;

	AddStream(new CAfxSingleStream(streamName, new CAfxRenderViewStream()));
}

void CAfxStreams::Console_AddBaseFxStream(const char * streamName)
{
	if(!Console_CheckStreamName(streamName))
		return;

	AddStream(new CAfxSingleStream(streamName, new CAfxBaseFxStream()));
}

void CAfxStreams::Console_AddDepthStream(const char * streamName)
{
	Tier0_Warning("Warning: Due to CS:GO 17th Ferbuary 2016 update this stream is not working perfectly.\n");

	if(!Console_CheckStreamName(streamName))
		return;

	AddStream(new CAfxSingleStream(streamName, new CAfxDepthStream()));
}

void CAfxStreams::Console_AddMatteWorldStream(const char * streamName)
{
	Tier0_Warning("Warning: Due to CS:GO 17th Ferbuary 2016 update this stream is not working perfectly.\n");

	if(!Console_CheckStreamName(streamName))
		return;

	AddStream(new CAfxSingleStream(streamName, new CAfxMatteWorldStream()));
}

void CAfxStreams::Console_AddDepthWorldStream(const char * streamName)
{
	Tier0_Warning("Warning: Due to CS:GO 17th Ferbuary 2016 update this stream is not working perfectly.\n");

	if(!Console_CheckStreamName(streamName))
		return;

	AddStream(new CAfxSingleStream(streamName, new CAfxDepthWorldStream()));
}

void CAfxStreams::Console_AddMatteEntityStream(const char * streamName)
{
	Tier0_Warning("Warning: Due to CS:GO 17th Ferbuary 2016 update this stream is not working perfectly.\n");

	if(!Console_CheckStreamName(streamName))
		return;

	AddStream(new CAfxSingleStream(streamName, new CAfxMatteEntityStream()));

	Tier0_Warning("Your matteEntity stream has been added, however please note that the alphaMatte + alphaEntity streams (combined in alphaMatteEntity stream) are far superior, because they can handle transparency far better.\n");
}

void CAfxStreams::Console_AddDepthEntityStream(const char * streamName)
{
	Tier0_Warning("Warning: Due to CS:GO 17th Ferbuary 2016 update this stream is not working perfectly.\n");

	if(!Console_CheckStreamName(streamName))
		return;

	AddStream(new CAfxSingleStream(streamName, new CAfxDepthEntityStream()));
}

void CAfxStreams::Console_AddAlphaMatteStream(const char * streamName)
{
	Tier0_Warning("Warning: Due to CS:GO 17th Ferbuary 2016 update this stream is not working perfectly.\n");

	if(!Console_CheckStreamName(streamName))
		return;

	AddStream(new CAfxSingleStream(streamName, new CAfxAlphaMatteStream()));
}

void CAfxStreams::Console_AddAlphaEntityStream(const char * streamName)
{
	Tier0_Warning("Warning: Due to CS:GO 17th Ferbuary 2016 update this stream is not working perfectly.\n");

	if(!Console_CheckStreamName(streamName))
		return;

	AddStream(new CAfxSingleStream(streamName, new CAfxAlphaEntityStream()));
}

void CAfxStreams::Console_AddAlphaWorldStream(const char * streamName)
{
	Tier0_Warning("Warning: Due to CS:GO 17th Ferbuary 2016 update this stream is not working perfectly.\n");

	if(!Console_CheckStreamName(streamName))
		return;

	AddStream(new CAfxSingleStream(streamName, new CAfxAlphaWorldStream()));
}

void CAfxStreams::Console_AddAlphaMatteEntityStream(const char * streamName)
{
	Tier0_Warning("Warning: Due to CS:GO 17th Ferbuary 2016 update this stream is not working perfectly.\n");

	if(!Console_CheckStreamName(streamName))
		return;

	AddStream(new CAfxTwinStream(streamName, new CAfxAlphaMatteStream(), new CAfxAlphaEntityStream(), CAfxTwinStream::SCT_ARedAsAlphaBColor));
}

void CAfxStreams::Console_PrintStreams()
{
	Tier0_Msg("index: name -> recorded?\n");
	int index = 0;
	for(std::list<CAfxRecordStream *>::iterator it = m_Streams.begin(); it != m_Streams.end(); ++it)
	{
		Tier0_Msg("%i: %s -> %s\n", index, (*it)->StreamName_get(), (*it)->Record_get() ? "RECORD ON (1)" : "record off (0)");
		++index;
	}
	Tier0_Msg(
		"=== Total streams: %i ===\n",
		index
	);
}

void CAfxStreams::Console_RemoveStream(const char * streamName)
{
	for(std::list<CAfxRecordStream *>::iterator it = m_Streams.begin(); it != m_Streams.end(); ++it)
	{
		if(!_stricmp(streamName, (*it)->StreamName_get()))
		{
			CAfxRecordStream * cur = *it;

			if(m_Recording) cur->RecordEnd();

			if(m_PreviewStream == cur) m_PreviewStream = 0;

			FinishStreamForGlowOverlayFix(cur);

			m_Streams.erase(it);

			cur->Release();

			return;
		}
	}
	Tier0_Msg("Error: invalid streamName.\n");
}

void CAfxStreams::Console_PreviewStream(const char * streamName)
{
	if(StringIsEmpty(streamName))
	{
		if(m_PreviewStream)
		{
			if(!m_Recording) RestoreMatVars();
		}
		m_PreviewStream = 0;
		return;
	}

	for(std::list<CAfxRecordStream *>::iterator it = m_Streams.begin(); it != m_Streams.end(); ++it)
	{
		if(!_stricmp(streamName, (*it)->StreamName_get()))
		{
			if(!(*it)->AsAfxSingleStream())
			{
				Tier0_Msg("Error: Only simple (single) streams can be previewed.\n");
				return;
			}

			CAfxRecordStream * cur = *it;
			m_PreviewStream = cur;
			if(!m_Recording) BackUpMatVars();
			SetMatVarsForStreams();
			return;
		}
	}
	Tier0_Msg("Error: invalid streamName.\n");
}

void CAfxStreams::Console_ListActions(void)
{
	CAfxBaseFxStream::Console_ListActions();
}

#define CAFXBASEFXSTREAM_STREAMCOMBINETYPES "aRedAsAlphaBColor|aColorBRedAsAlpha"
#define CAFXBASEFXSTREAM_STREAMCAPTURETYPES "normal|depth24|depth24ZIP|depthF|depthFZIP"
#define CAFXSTREAMS_ACTIONSUFFIX " <actionName> - Set action with name <actionName> (see mirv_streams actions)."

void CAfxStreams::Console_EditStream(const char * streamName, IWrpCommandArgs * args)
{
	bool streamEdited = false;

	std::string sInputName(streamName);
	std::transform(sInputName.begin(), sInputName.end(), sInputName.begin(), ::tolower);

	for (std::list<CAfxRecordStream *>::iterator it = m_Streams.begin(); it != m_Streams.end(); ++it)
	{
		char const * targetName = (*it)->StreamName_get();
		std::string sTargetName(targetName);
		std::transform(sTargetName.begin(), sTargetName.end(), sTargetName.begin(), ::tolower);

		if (StringWildCard1Matched(sInputName.c_str(), sTargetName.c_str()))
		{
			Tier0_Msg("--- Editing stream \"%s\" ----\n", targetName);
			Console_EditStream((*it), args);
			streamEdited = true;
		}
	}

	if(!streamEdited)
		Tier0_Warning("Error: No streamName matches \"%s\".\n", streamName);
}

void CAfxStreams::Console_EditStream(CAfxStream * stream, IWrpCommandArgs * args)
{
	CAfxStream * cur = stream;

	CAfxRecordStream * curRecord = 0;
	CAfxSingleStream * curSingle = 0;
	CAfxTwinStream * curTwin = 0;
	CAfxRenderViewStream * curRenderView = 0;

	CAfxStreamSharedInterLock afxStreamInterLock(cur);
	
	if(cur)
	{
		curRecord = cur->AsAfxRecordStream();
	
		if(curRecord)
		{
			curSingle = curRecord->AsAfxSingleStream();
			curTwin = curRecord->AsAfxTwinStream();

			if(curSingle)
			{
				curRenderView = curSingle->Stream_get();
			}
		}
	}

	char const * cmdPrefix = args->ArgV(0);

	int argcOffset = 1;

	int argc = args->ArgC() - argcOffset;

	if(cur)
	{
	}

	if(curSingle)
	{
	}

	if(curTwin)
	{
		if(1 <= argc)
		{
			char const * cmd0 = args->ArgV(argcOffset +0);

			if(!_stricmp(cmd0, "streamA"))
			{
				CSubWrpCommandArgs subArgs(args, argcOffset + 1);

				Console_EditStream(curTwin->StreamA_get(), &subArgs);
				return;
			}
			else
			if(!_stricmp(cmd0, "streamB"))
			{
				CSubWrpCommandArgs subArgs(args, argcOffset + 1);

				Console_EditStream(curTwin->StreamB_get(), &subArgs);
				return;
			}
			else
			if(!_stricmp(cmd0, "streamCombineType"))
			{
				if(2 <= argc)
				{
					char const * cmd1 = args->ArgV(argcOffset +1);
					CAfxTwinStream::StreamCombineType value;

					if(Console_ToStreamCombineType(cmd1, value))
					{
						curTwin->StreamCombineType_set(value);
						return;
					}
				}

				Tier0_Msg(
					"%s streamCombineType " CAFXBASEFXSTREAM_STREAMCOMBINETYPES " - Set new combine type.\n"
					"Current value: %s.\n"
					, cmdPrefix
					, Console_FromStreamCombineType(curTwin->StreamCombineType_get())
				);
				return;
			}
		}
	}

	if(curRecord)
	{
		if(1 <= argc)
		{
			char const * cmd0 = args->ArgV(argcOffset +0);

			if(!_stricmp(cmd0, "record"))
			{
				if(2 <= argc)
				{
					char const * cmd1 = args->ArgV(argcOffset +1);

					curRecord->Record_set(atoi(cmd1) != 0 ? true : false);

					return;
				}

				Tier0_Msg(
					"%s record 0|1 - Whether to record this stream with mirv_streams record - 0 = record off, 1 = RECORD ON.\n"
					"Current value: %s.\n"
					, cmdPrefix
					, curRecord->Record_get() ? "1" : "0"
				);
				return;
			}
		}
	}

	if(curRenderView)
	{
		if (Console_EditStream(curRenderView, args))
			return;
	}		

	if (curTwin)
	{
		Tier0_Msg("-- twin properties --\n");
		Tier0_Msg("%s streamA [...] - Edit sub stream A.\n", cmdPrefix);
		Tier0_Msg("%s streamB [...] - Edit sub stream B.\n", cmdPrefix);
		Tier0_Msg("%s streamCombineType [...] - Controlls how streams are combined.\n", cmdPrefix);
	}

	if (curSingle)
	{
	}

	if (curRecord)
	{
		Tier0_Msg("-- record properties --\n");
		Tier0_Msg("%s record [...] - Controlls whether or not this stream is recorded with mirv_streams record.\n", cmdPrefix);
	}

	if (cur)
	{
	}
	Tier0_Msg("== No more properties. ==\n");
}

bool CAfxStreams::Console_EditStream(CAfxRenderViewStream * stream, IWrpCommandArgs * args)
{
	CAfxRenderViewStream * curRenderView = stream;
	CAfxBaseFxStream * curBaseFx = 0;
	
	CAfxStreamSharedInterLock afxRenderViewStreamInterLock(curRenderView);

	if(curRenderView)
	{
		curBaseFx = curRenderView->AsAfxBaseFxStream();
	}

	char const * cmdPrefix = args->ArgV(0);

	int argcOffset = 1;

	int argc = args->ArgC() - argcOffset;

	if(curRenderView)
	{
		if(1 <= argc)
		{
			char const * cmd0 = args->ArgV(argcOffset +0);

			if(!_stricmp(cmd0, "attachCommands"))
			{
				if(2 <= argc)
				{
					std::string value;

					for(int i=argcOffset +1; i < args->ArgC(); ++i)
					{
						if(argcOffset +1 < i)
						{
							value.append(" ");
						}
						value.append(args->ArgV(i));
					}
					
					curRenderView->AttachCommands_set(value.c_str());
					return true;
				}

				Tier0_Msg(
					"%s attachCommands <commandString1> [<commandString2>] ... [<commandStringN>] - Set command strings to be executed when stream is attached.\n"
					"Current value: %s.\n"
					, cmdPrefix
					, curRenderView->AttachCommands_get()
				);
				return true;
			}
			else
			if(!_stricmp(cmd0, "detachCommands"))
			{
				if(2 <= argc)
				{
					std::string value;

					for(int i=argcOffset +1; i < args->ArgC(); ++i)
					{
						if(argcOffset +1 < i)
						{
							value.append(" ");
						}
						value.append(args->ArgV(i));
					}
					
					curRenderView->DetachCommands_set(value.c_str());
					return true;
				}

				Tier0_Msg(
					"%s detachCommands <commandString1> [<commandString2>] ... [<commandStringN>] - Set command strings to be executed when stream is detached.\n"
					"Current value: %s.\n"
					, cmdPrefix
					, curRenderView->DetachCommands_get()
				);
				return true;
			}
			else
			if(!_stricmp(cmd0, "drawHud"))
			{
				if(2 <= argc)
				{
					char const * cmd1 = args->ArgV(argcOffset +1);

					curRenderView->DrawHud_set(atoi(cmd1) != 0 ? true : false);

					return true;
				}

				Tier0_Msg(
					"%s drawHud 0|1 - Whether to draw HUD for this stream - 0 = don't draw, 1 = draw.\n"
					"Current value: %s.\n"
					, cmdPrefix
					, curRenderView->DrawHud_get() ? "1" : "0"
				);
				return true;
			}
			else
			if(!_stricmp(cmd0, "drawViewModel"))
			{
				if(2 <= argc)
				{
					char const * cmd1 = args->ArgV(argcOffset +1);

					curRenderView->DrawViewModel_set(atoi(cmd1) != 0 ? true : false);

					return true;
				}

				Tier0_Msg(
					"%s drawViewModel 0|1 - Whether to draw view model (in-eye weapon) for this stream - 0 = don't draw, 1 = draw.\n"
					"Current value: %s.\n"
					, cmdPrefix
					, curRenderView->DrawViewModel_get() ? "1" : "0"
				);
				return true;
			}
			else
			if(!_stricmp(cmd0, "captureType"))
			{
				if(2 <= argc)
				{
					char const * cmd1 = args->ArgV(argcOffset +1);
					CAfxRenderViewStream::StreamCaptureType value;

					if(Console_ToStreamCaptureType(cmd1, value))
					{
						if(value == CAfxRenderViewStream::SCT_DepthF || CAfxRenderViewStream::SCT_DepthFZIP)
						{
							if(!(AfxD3D9_Check_Supports_R32F_With_Blending() && m_RenderTargetDepthF))
							{
								Tier0_Warning("AFXERROR: This capture type ist not fully supported according to your graphics card / driver!\n");
							}
						}
						curRenderView->StreamCaptureType_set(value);
						return true;
					}
				}

				Tier0_Msg(
					"%s captureType " CAFXBASEFXSTREAM_STREAMCAPTURETYPES " - Set new render type.\n"
					"Current value: %s.\n"
					, cmdPrefix
					, Console_FromStreamCaptureType(curRenderView->StreamCaptureType_get())
				);
				return true;
			}
		}
	}

	if(curBaseFx)
	{
		if(1 <= argc)
		{
			char const * cmd0 = args->ArgV(argcOffset +0);

			if (!_stricmp(cmd0, "picker"))
			{
				if (2 <= argc)
				{
					char const * cmd1 = args->ArgV(argcOffset + 1);

					if (!_stricmp(cmd1, "ent") && 3 <= argc)
					{
						bool value = 0 != atoi(args->ArgV(argcOffset + 2));
						curBaseFx->Picker_Pick(true, value);
						return true;
					}
					else
					if (!_stricmp(cmd1, "mat") && 3 <= argc)
					{
						bool value = 0 != atoi(args->ArgV(argcOffset + 2));
						curBaseFx->Picker_Pick(false, value);
						return true;
					}
					else
					if (!_stricmp(cmd1, "print"))
					{
						curBaseFx->Picker_Print();
						return true;
					}
					else
					if (!_stricmp(cmd1, "stop"))
					{
						curBaseFx->Picker_Stop();
						return true;
					}
				}

				Tier0_Msg(
					"%s picker ent 0|1 - Tell picker if entity is visible (1) or not (0). (Or start picking with 1.)\n"
					"%s picker mat 0|1 - Tell picker if material is visible (1) or not (0). (Or start picking with 1.)\n"
					"%s picker print - Prints currently picked result set (entities / materials).\n"
					"%s picker stop - Stop picking.\n"
					"ATTENTION: Stream needs to be in preview of course, otherwise you won't see anything ;)\n"
					"ATTENTION: Do not forget to stop the picker, otherwise you might have some surprises ;)\n"
					, cmdPrefix
					, cmdPrefix
					, cmdPrefix
					, cmdPrefix
					, cmdPrefix
				);

				return true;
			}
			else
			if(!_stricmp(cmd0, "actionFilter"))
			{
				if(2 <= argc)
				{
					char const * cmd1 = args->ArgV(argcOffset +1);

					if(!_stricmp(cmd1, "add"))
					{
						if(4 <= argc)
						{
							char const * cmd2 = args->ArgV(argcOffset +2);
							char const * cmd3 = args->ArgV(argcOffset +3);

							CAfxBaseFxStream::CAction * value;

							if(Console_ToAfxAction(cmd3, value))
							{
								curBaseFx->Console_ActionFilter_Add(cmd2, value);
								return true;
							}
						}

						Tier0_Msg(
							"%s actionFilter add <materialNameMask> <actionName> - Add a new filter action.\n"
							"\t<materialNameMask> - material name/path to match, where \\* = wildcard and \\\\ = \\\n"
							"\tTo find a material name/path enable debugPrint and use invalidateMap command in stream options!"
							"\t<actionName> - name of action (see mirv_actions).\n"
							, cmdPrefix
						);
						return true;
					}
					else if (!_stricmp("addEx", cmd1))
					{
						CSubWrpCommandArgs subArgs(args, 3);
						curBaseFx->Console_ActionFilter_AddEx(this, &subArgs);
						return true;
					}
					else
					if(!_stricmp(cmd1, "print"))
					{
						curBaseFx->Console_ActionFilter_Print();
						return true;
					}
					else
					if(!_stricmp(cmd1, "remove"))
					{
						if(3 <= argc)
						{
							char const * cmd2 = args->ArgV(argcOffset +2);

							curBaseFx->Console_ActionFilter_Remove(atoi(cmd2));

							return true;
						}
						
						Tier0_Msg(
							"%s actionFilter remove <actionId> - Removes action with id number <actionId>.\n"
							, cmdPrefix
						);
						return true;
					}
					else
					if(!_stricmp(cmd1, "move"))
					{
						if(4 <= argc)
						{
							char const * cmd2 = args->ArgV(argcOffset +2);
							char const * cmd3 = args->ArgV(argcOffset +3);


							curBaseFx->Console_ActionFilter_Move(atoi(cmd2), atoi(cmd3));

							return true;
						}
						
						Tier0_Msg(
							"%s actionFilter move <actionId> <beforeId> - Moves action with id number <actionId> before action with id <beforeId> (<beforeId> value can be 1 greater than the last id to move at the end).\n"
							, cmdPrefix
						);
						return true;
					}
				}

				Tier0_Msg(
					"%s actionFilter add [...] - Add a new filter action.\n"
					"%s actionFilter addEx [...] - Add a new filter action.\n"
					"%s actionFilter print - Print current filter actions.\n"
					"%s actionFilter remove [...] - Remove a filter action.\n"
					"%s actionFilter move [...] - Move filter action (change priority).\n"
					, cmdPrefix
					, cmdPrefix
					, cmdPrefix
					, cmdPrefix
					, cmdPrefix
				);
				return true;
			}
			else
			if(!_stricmp(cmd0, "clientEffectTexturesAction"))
			{
				if(2 <= argc)
				{
					char const * cmd1 = args->ArgV(argcOffset +1);
					CAfxBaseFxStream::CAction * value;

					if(Console_ToAfxAction(cmd1, value))
					{
						curBaseFx->ClientEffectTexturesAction_set(value);
						return true;
					}
				}

				Tier0_Msg(
					"%s clientEffectTexturesAction" CAFXSTREAMS_ACTIONSUFFIX "\n"
					"Current value: %s.\n"
					, cmdPrefix
					, Console_FromAfxAction(curBaseFx->ClientEffectTexturesAction_get())
				);
				return true;
			}
			else
			if(!_stricmp(cmd0, "worldTexturesAction"))
			{
				if(2 <= argc)
				{
					char const * cmd1 = args->ArgV(argcOffset +1);
					CAfxBaseFxStream::CAction * value;

					if(Console_ToAfxAction(cmd1, value))
					{
						curBaseFx->WorldTexturesAction_set(value);
						return true;
					}
				}

				Tier0_Msg(
					"%s worldTexturesAction" CAFXSTREAMS_ACTIONSUFFIX "\n"
					"Current value: %s.\n"
					, cmdPrefix
					, Console_FromAfxAction(curBaseFx->WorldTexturesAction_get())
				);
				return true;
			}
			else
			if(!_stricmp(cmd0, "skyBoxTexturesAction"))
			{
				if(2 <= argc)
				{
					char const * cmd1 = args->ArgV(argcOffset +1);
					CAfxBaseFxStream::CAction * value;

					if(Console_ToAfxAction(cmd1, value))
					{
						curBaseFx->SkyBoxTexturesAction_set(value);
						return true;
					}
				}

				Tier0_Msg(
					"%s skyBoxTexturesAction" CAFXSTREAMS_ACTIONSUFFIX "\n"
					"Current value: %s.\n"
					, cmdPrefix
					, Console_FromAfxAction(curBaseFx->SkyBoxTexturesAction_get())
				);
				return true;
			}
			else
			if(!_stricmp(cmd0, "staticPropTexturesAction"))
			{
				if(2 <= argc)
				{
					char const * cmd1 = args->ArgV(argcOffset +1);
					CAfxBaseFxStream::CAction * value;

					if(Console_ToAfxAction(cmd1, value))
					{
						curBaseFx->StaticPropTexturesAction_set(value);
						return true;
					}
				}

				Tier0_Msg(
					"%s staticPropTexturesAction" CAFXSTREAMS_ACTIONSUFFIX "\n"
					"Current value: %s.\n"
					, cmdPrefix
					, Console_FromAfxAction(curBaseFx->StaticPropTexturesAction_get())
				);
				return true;
			}
			else
			if(!_stricmp(cmd0, "cableAction"))
			{
				if(2 <= argc)
				{
					char const * cmd1 = args->ArgV(argcOffset +1);
					CAfxBaseFxStream::CAction * value;

					if(Console_ToAfxAction(cmd1, value))
					{
						curBaseFx->CableAction_set(value);
						return true;
					}
				}

				Tier0_Msg(
					"%s cableAction" CAFXSTREAMS_ACTIONSUFFIX "\n"
					"Current value: %s.\n"
					, cmdPrefix
					, Console_FromAfxAction(curBaseFx->CableAction_get())
				);
				return true;
			}
			else
			if(!_stricmp(cmd0, "playerModelsAction"))
			{
				if(2 <= argc)
				{
					char const * cmd1 = args->ArgV(argcOffset +1);
					CAfxBaseFxStream::CAction * value;

					if(Console_ToAfxAction(cmd1, value))
					{
						curBaseFx->PlayerModelsAction_set(value);
						return true;
					}
				}

				Tier0_Msg(
					"%s playerModelsAction" CAFXSTREAMS_ACTIONSUFFIX "\n"
					"Current value: %s.\n"
					, cmdPrefix
					, Console_FromAfxAction(curBaseFx->PlayerModelsAction_get())
				);
				return true;
			}
			else
			if(!_stricmp(cmd0, "weaponModelsAction"))
			{
				if(2 <= argc)
				{
					char const * cmd1 = args->ArgV(argcOffset +1);
					CAfxBaseFxStream::CAction * value;

					if(Console_ToAfxAction(cmd1, value))
					{
						curBaseFx->WeaponModelsAction_set(value);
						return true;
					}
				}

				Tier0_Msg(
					"%s weaponModelsAction" CAFXSTREAMS_ACTIONSUFFIX "\n"
					"Current value: %s.\n"
					, cmdPrefix
					, Console_FromAfxAction(curBaseFx->WeaponModelsAction_get())
				);
				return true;
			}
			else
			if(!_stricmp(cmd0, "statTrakAction")||!_stricmp(cmd0, "stattrackAction"))
			{
				if(2 <= argc)
				{
					char const * cmd1 = args->ArgV(argcOffset +1);
					CAfxBaseFxStream::CAction * value;

					if(Console_ToAfxAction(cmd1, value))
					{
						curBaseFx->StatTrakAction_set(value);
						return true;
					}
				}

				Tier0_Msg(
					"%s statTrakAction" CAFXSTREAMS_ACTIONSUFFIX "\n"
					"Current value: %s.\n"
					, cmdPrefix
					, Console_FromAfxAction(curBaseFx->StatTrakAction_get())
				);
				return true;
			}
			else
			if(!_stricmp(cmd0, "shellModelsAction"))
			{
				if(2 <= argc)
				{
					char const * cmd1 = args->ArgV(argcOffset +1);
					CAfxBaseFxStream::CAction * value;

					if(Console_ToAfxAction(cmd1, value))
					{
						curBaseFx->ShellModelsAction_set(value);
						return true;
					}
				}

				Tier0_Msg(
					"%s shellModelsAction" CAFXSTREAMS_ACTIONSUFFIX "\n"
					"Current value: %s.\n"
					, cmdPrefix
					, Console_FromAfxAction(curBaseFx->ShellModelsAction_get())
				);
				return true;
			}
			else
			if(!_stricmp(cmd0, "otherModelsAction"))
			{
				if(2 <= argc)
				{
					char const * cmd1 = args->ArgV(argcOffset +1);
					CAfxBaseFxStream::CAction * value;

					if(Console_ToAfxAction(cmd1, value))
					{
						curBaseFx->OtherModelsAction_set(value);
						return true;
					}
				}

				Tier0_Msg(
					"%s otherModelsAction" CAFXSTREAMS_ACTIONSUFFIX "\n"
					"Current value: %s.\n"
					, cmdPrefix
					, Console_FromAfxAction(curBaseFx->OtherModelsAction_get())
				);
				return true;
			}
			else
			if(!_stricmp(cmd0, "decalTexturesAction"))
			{
				if(2 <= argc)
				{
					char const * cmd1 = args->ArgV(argcOffset +1);
					CAfxBaseFxStream::CAction * value;

					if(Console_ToAfxAction(cmd1, value))
					{
						curBaseFx->DecalTexturesAction_set(value);
						return true;
					}
				}

				Tier0_Msg(
					"%s decalTexturesAction" CAFXSTREAMS_ACTIONSUFFIX "\n"
					"Current value: %s.\n"
					, cmdPrefix
					, Console_FromAfxAction(curBaseFx->DecalTexturesAction_get())
				);
				return true;
			}
			else
			if(!_stricmp(cmd0, "effectsAction"))
			{
				if(2 <= argc)
				{
					char const * cmd1 = args->ArgV(argcOffset +1);
					CAfxBaseFxStream::CAction * value;

					if(Console_ToAfxAction(cmd1, value))
					{
						curBaseFx->EffectsAction_set(value);
						return true;
					}
				}

				Tier0_Msg(
					"%s effectsAction" CAFXSTREAMS_ACTIONSUFFIX "\n"
					"Current value: %s.\n"
					, cmdPrefix
					, Console_FromAfxAction(curBaseFx->EffectsAction_get())
				);
				return true;
			}
			else
			if(!_stricmp(cmd0, "shellParticleAction"))
			{
				if(2 <= argc)
				{
					char const * cmd1 = args->ArgV(argcOffset +1);
					CAfxBaseFxStream::CAction * value;

					if(Console_ToAfxAction(cmd1, value))
					{
						curBaseFx->ShellParticleAction_set(value);
						return true;
					}
				}

				Tier0_Msg(
					"%s shellParticleAction" CAFXSTREAMS_ACTIONSUFFIX "\n"
					"Current value: %s.\n"
					, cmdPrefix
					, Console_FromAfxAction(curBaseFx->ShellParticleAction_get())
				);
				return true;
			}
			else
			if(!_stricmp(cmd0, "otherParticleAction"))
			{
				if(2 <= argc)
				{
					char const * cmd1 = args->ArgV(argcOffset +1);
					CAfxBaseFxStream::CAction * value;

					if(Console_ToAfxAction(cmd1, value))
					{
						curBaseFx->OtherParticleAction_set(value);
						return true;
					}
				}

				Tier0_Msg(
					"%s otherParticleAction" CAFXSTREAMS_ACTIONSUFFIX "\n"
					"Current value: %s.\n"
					, cmdPrefix
					, Console_FromAfxAction(curBaseFx->OtherParticleAction_get())
				);
				return true;
			}
			else
			if(!_stricmp(cmd0, "stickerAction"))
			{
				if(2 <= argc)
				{
					char const * cmd1 = args->ArgV(argcOffset +1);
					CAfxBaseFxStream::CAction * value;

					if(Console_ToAfxAction(cmd1, value))
					{
						curBaseFx->StickerAction_set(value);
						return true;
					}
				}

				Tier0_Msg(
					"%s stickerAction" CAFXSTREAMS_ACTIONSUFFIX "\n"
					"Current value: %s.\n"
					, cmdPrefix
					, Console_FromAfxAction(curBaseFx->StickerAction_get())
				);
				return true;
			}
			else
			if(!_stricmp(cmd0, "errorMaterialAction"))
			{
				if(2 <= argc)
				{
					char const * cmd1 = args->ArgV(argcOffset +1);
					CAfxBaseFxStream::CAction * value;

					if(Console_ToAfxAction(cmd1, value))
					{
						curBaseFx->ErrorMaterialAction_set(value);
						return true;
					}
				}

				Tier0_Msg(
					"%s errorMaterialAction" CAFXSTREAMS_ACTIONSUFFIX "\n"
					"Current value: %s.\n"
					, cmdPrefix
					, Console_FromAfxAction(curBaseFx->ErrorMaterialAction_get())
				);
				return true;
			}
			else
			if(!_stricmp(cmd0, "otherAction"))
			{
				if(2 <= argc)
				{
					char const * cmd1 = args->ArgV(argcOffset +1);
					CAfxBaseFxStream::CAction * value;

					if(Console_ToAfxAction(cmd1, value))
					{
						curBaseFx->OtherAction_set(value);
						return true;
					}
				}

				Tier0_Msg(
					"%s otherAction" CAFXSTREAMS_ACTIONSUFFIX "\n"
					"Current value: %s.\n"
					, cmdPrefix
					, Console_FromAfxAction(curBaseFx->OtherAction_get())
				);
				return true;
			}
			else
			if(!_stricmp(cmd0, "writeZAction"))
			{
				if(2 <= argc)
				{
					char const * cmd1 = args->ArgV(argcOffset +1);
					CAfxBaseFxStream::CAction * value;

					if(Console_ToAfxAction(cmd1, value))
					{
						curBaseFx->WriteZAction_set(value);
						return true;
					}
				}

				Tier0_Msg(
					"%s writeZAction" CAFXSTREAMS_ACTIONSUFFIX "\n"
					"Current value: %s.\n"
					, cmdPrefix
					, Console_FromAfxAction(curBaseFx->WriteZAction_get())
				);
				return true;
			}
			else
			if(!_stricmp(cmd0, "devAction"))
			{
				Tier0_Msg(
					"%s devAction\n"
					"Current value: %s.\n"
					, cmdPrefix
					, Console_FromAfxAction(curBaseFx->DevAction_get())
				);
				return true;
			}
			else
			if(!_stricmp(cmd0, "otherEngineAction"))
			{
				Tier0_Msg(
					"%s otherEngineAction\n"
					"Current value: %s.\n"
					, cmdPrefix
					, Console_FromAfxAction(curBaseFx->OtherEngineAction_get())
				);
				return true;
			}
			else
			if(!_stricmp(cmd0, "otherSpecialAction"))
			{
				Tier0_Msg(
					"%s otherSpecialAction\n"
					"Current value: %s.\n"
					, cmdPrefix
					, Console_FromAfxAction(curBaseFx->OtherSpecialAction_get())
				);
				return true;
			}
			else
			if(!_stricmp(cmd0, "vguiAction"))
			{
				Tier0_Msg(
					"%s vguiAction\n"
					"Current value: %s.\n"
					, cmdPrefix
					, Console_FromAfxAction(curBaseFx->VguiAction_get())
				);
				return true;
			}
			else
			if(!_stricmp(cmd0, "depthVal"))
			{
				if(2 <= argc)
				{
					char const * cmd1 = args->ArgV(argcOffset +1);
					curBaseFx->DepthVal_set((float)atof(cmd1));
					return true;
				}

				Tier0_Msg(
					"%s depthVal <fValue> - Set new miniumum depth floating point value <fValue>.\n"
					"Current value: %f.\n"
					, cmdPrefix
					, curBaseFx->DepthVal_get()
				);
				return true;
			}
			else
			if(!_stricmp(cmd0, "depthValMax"))
			{
				if(2 <= argc)
				{
					char const * cmd1 = args->ArgV(argcOffset +1);
					curBaseFx->DepthValMax_set((float)atof(cmd1));
					return true;
				}

				Tier0_Msg(
					"%s depthValMax <fValue> - Set new maximum depth floating point value <fValue>.\n"
					"Current value: %f.\n"
					, cmdPrefix
					, curBaseFx->DepthValMax_get()
				);
				return true;
			}
			else
			if(!_stricmp(cmd0, "smokeOverlayAlphaFactor"))
			{
				if(2 <= argc)
				{
					char const * cmd1 = args->ArgV(argcOffset +1);
					curBaseFx->SmokeOverlayAlphaFactor_set((float)atof(cmd1));
					return true;
				}

				Tier0_Msg(
					"%s smokeOverlayAlphaFactor <fValue> - Set new factor that is multiplied with smoke overlay alpha (i.e. 0 would disable it completely).\n"
					"Current value: %f.\n"
					, cmdPrefix
					, curBaseFx->SmokeOverlayAlphaFactor_get()
				);
				return true;
			}
			else
				if (!_stricmp(cmd0, "shouldForceNoVisOverride"))
				{
					if (!Hook_csgo_CViewRender_ShouldForceNoVis())
					{
						Tier0_Warning("%s smokeOverlayAlphaFactor - ERROR: Required hook not available, will not work.\n", cmdPrefix);
					}

					if (2 <= argc)
					{
						char const * cmd1 = args->ArgV(argcOffset + 1);
						curBaseFx->ShouldForceNoVisOverride_set(0 != atoi(cmd1));
						return true;
					}

					Tier0_Msg(
						"%s shouldForceNoVisOverride 0|1 - Useful for wallhack: If to force ShouldForceNoVisOverride to return true.\n"
						"Current value: %i.\n"
						, cmdPrefix
						, curBaseFx->ShouldForceNoVisOverride_get() ? 1 : 0
					);
					return true;
				}
				else
			if(!_stricmp(cmd0, "debugPrint"))
			{
				if(2 <= argc)
				{
					char const * cmd1 = args->ArgV(argcOffset +1);

					curBaseFx->DebugPrint_set(0 != atoi(cmd1) ? true : false);
					return true;
				}

				Tier0_Msg(
					"%s debugPrint 0|1 - Disable / enable debug console output.\n"
					"Current value: %s.\n"
					, cmdPrefix
					, curBaseFx->DebugPrint_get() ? "1" : "0"
				);
				return true;
			}
			else
			if(!_stricmp(cmd0, "invalidateMap"))
			{
				curBaseFx->InvalidateMap();
				return true;
			}
			else
			if(!_stricmp(cmd0, "testAction"))
			{
				if(2 <= argc)
				{
					char const * cmd1 = args->ArgV(argcOffset +1);

					curBaseFx->TestAction_set(0 != atoi(cmd1) ? true : false);
					return true;
				}

				Tier0_Msg(
					"%s testAction 0|1 - Disable / enable action for devloper testing purposes.\n"
					"Current value: %s.\n"
					, cmdPrefix
					, curBaseFx->TestAction_get() ? "1" : "0"
				);
				return true;
			}
			else
			if(!_stricmp(cmd0, "man"))
			{
				/*
				if(2 <= argc)
				{
					char const * cmd1 = args->ArgV(argcOffset +1);

					if(!_stricmp(cmd1, "toDepth"))
					{
						curBaseFx->ConvertStreamDepth(false, false);
						return;
					}
					else
					if(!_stricmp(cmd1, "toDepth24"))
					{
						curBaseFx->ConvertStreamDepth(true, false);
						return;
					}
					else
					if(!_stricmp(cmd1, "toDepth24ZIP"))
					{
						curBaseFx->ConvertStreamDepth(true, true);
						return;
					}
				}

				Tier0_Msg(
					"%s man toDepth - Convert drawDepth24 actions to drawDepth and set captureType normal.\n"
					"%s man toDepth24 - Convert drawDepth actions to drawDepth24 and set captureType depth24.\n"
					"%s man toDepth24ZIP - Convert drawDepth actions to drawDepth24 and set captureType depth24ZIP (might be slower than depth24).\n"
					, cmdPrefix
					, cmdPrefix
					, cmdPrefix
				);*/
				Tier0_Warning("Warning: Due to CS:GO 17th Ferbuary 2016 update this feature is not available.\n");
				return true;
			}
		}
	}

	if(curBaseFx)
	{
		Tier0_Msg("-- baseFx properties --\n");
		Tier0_Msg("%s picker [...] - Helps picking a visible material / entity.\n", cmdPrefix);
		Tier0_Msg("%s actionFilter [...] - Set actions by material name (not safe maybe).\n", cmdPrefix);
		Tier0_Msg("%s clientEffectTexturesAction [...]\n", cmdPrefix);
		Tier0_Msg("%s worldTexturesAction [...]\n", cmdPrefix);
		Tier0_Msg("%s skyBoxTexturesAction [...]\n", cmdPrefix);
		Tier0_Msg("%s staticPropTexturesAction [...]\n", cmdPrefix);
		Tier0_Msg("%s cableAction [...]\n", cmdPrefix);
		Tier0_Msg("%s playerModelsAction [...]\n", cmdPrefix);
		Tier0_Msg("%s weaponModelsAction [...]\n", cmdPrefix);
		Tier0_Msg("%s statTrakAction [...]\n", cmdPrefix);
		Tier0_Msg("%s shellModelsAction [...]\n", cmdPrefix);
		Tier0_Msg("%s otherModelsAction [...]\n", cmdPrefix);
		Tier0_Msg("%s decalTexturesAction [...]\n", cmdPrefix);
		Tier0_Msg("%s effectsAction [...]\n", cmdPrefix);
		Tier0_Msg("%s shellParticleAction [...]\n", cmdPrefix);
		Tier0_Msg("%s otherParticleAction [...]\n", cmdPrefix);
		Tier0_Msg("%s stickerAction [...]\n", cmdPrefix);
		Tier0_Msg("%s errorMaterialAction [...]\n", cmdPrefix);
		Tier0_Msg("%s otherAction [...]\n", cmdPrefix);
		Tier0_Msg("%s writeZAction [...]\n", cmdPrefix);
		Tier0_Msg("%s devAction [...] - Readonly.\n", cmdPrefix);
		Tier0_Msg("%s otherEngineAction [...] - Readonly.\n", cmdPrefix);
		Tier0_Msg("%s otherSpecialAction [...] - Readonly.\n", cmdPrefix);
		Tier0_Msg("%s vguiAction [...] - Readonly.\n", cmdPrefix);
		Tier0_Msg("%s depthVal [...]\n", cmdPrefix);
		Tier0_Msg("%s depthValMax [...]\n", cmdPrefix);
		Tier0_Msg("%s smokeOverlayAlphaFactor [...]\n", cmdPrefix);
		Tier0_Msg("%s shouldForceNoVisOverride [...]\n", cmdPrefix);
		Tier0_Msg("%s debugPrint [...]\n", cmdPrefix);
		Tier0_Msg("%s invalidateMap - invaldiates the material map.\n", cmdPrefix);
		Tier0_Msg("%s man [...] - Manipulate stream more easily (i.e. depth to depth24).\n", cmdPrefix);
		// testAction options is not displayed, because we don't want users to use it.
		// Tier0_Msg("%s testAction [...]\n", cmdPrefix);
	}

	if (curRenderView)
	{
		Tier0_Msg("-- renderView properties --\n");
		Tier0_Msg("%s attachCommands [...] - Commands to be executed when stream is attached. WARNING. Use at your own risk, game may crash!\n", cmdPrefix);
		Tier0_Msg("%s detachCommands [...] - Commands to be executed when stream is detached. WARNING. Use at your own risk, game may crash!\n", cmdPrefix);
		Tier0_Msg("%s drawHud [...] - Controlls whether or not HUD is drawn for this stream.\n", cmdPrefix);
		Tier0_Msg("%s drawViewModel [...] - Controls whether or not view model (in-eye weapon) is drawn for this stream.\n", cmdPrefix);
		Tier0_Msg("%s captureType [...] - Stream capture type.\n", cmdPrefix);
	}

	return false;
}


void CAfxStreams::Console_Bvh(IWrpCommandArgs * args)
{
	int argc = args->ArgC();

	char const * prefix = args->ArgV(0);

	if (m_Recording)
	{
		Tier0_Warning("Error: These settings cannot be accessed during mirv_streams recording!\n");
		return;
	}

	if (2 <= argc)
	{
		char const * cmd1 = args->ArgV(1);

		if (!_stricmp(cmd1, "cam"))
		{
			if (3 <= argc)
			{
				char const * cmd2 = args->ArgV(2);

				m_CamBvh = 0 != atoi(cmd2);
				return;
			}

			Tier0_Msg(
				"%s cam 0|1 - Enable (1) / Disable (0) main camera export (overrides/uses mirv_camexport actually).\n"
				"Current value: %i.\n"
				, prefix
				, m_CamBvh ? 1 : 0
			);
			return;
		}
		else
		if (!_stricmp(cmd1, "ent"))
		{
			if (3 <= argc)
			{
				char const * cmd2 = args->ArgV(2);

				if (!_stricmp(cmd2, "add"))
				{
					if (6 <= argc)
					{
						char const * sIndex = args->ArgV(3);
						char const * sOrigin = args->ArgV(4);
						char const * sAngles = args->ArgV(5);

						int entityIndex = atoi(sIndex);

						CEntityBvhCapture::Origin_e origin = CEntityBvhCapture::O_View;
						if (!_stricmp(sOrigin, "net"))
						{
							origin = CEntityBvhCapture::O_Net;
						}
						else
						if (!_stricmp(sOrigin, "view"))
						{
							origin = CEntityBvhCapture::O_View;
						}
						else
						{
							Tier0_Warning("Error: invalid <originType>!\n");
							return;
						}

						CEntityBvhCapture::Angles_e angles = CEntityBvhCapture::A_View;
						if (!_stricmp(sAngles, "net"))
						{
							angles = CEntityBvhCapture::A_Net;
						}
						else
						if (!_stricmp(sAngles, "view"))
						{
							angles = CEntityBvhCapture::A_View;
						}
						else
						{
							Tier0_Warning("Error: invalid <anglesType>!\n");
							return;
						}

						CEntityBvhCapture * bvhEntityCapture = new CEntityBvhCapture(entityIndex, origin, angles);

						std::list<CEntityBvhCapture *>::iterator cur = m_EntityBvhCaptures.begin();

						while (cur != m_EntityBvhCaptures.end() && (*cur)->EntityIndex_get() < entityIndex)
							++cur;

						if (cur != m_EntityBvhCaptures.end() && (*cur)->EntityIndex_get() == entityIndex)
						{
							delete (*cur);
							m_EntityBvhCaptures.emplace(cur, bvhEntityCapture);
						}
						else
						{
							m_EntityBvhCaptures.insert(cur, bvhEntityCapture);
						}

						return;
					}

					Tier0_Msg(
						"%s ent add <entityIndex> <originType> <anglesType> - Add an entity to list, <originType> and <anglesType> can be \"net\" or \"view\".\n"
						, prefix
					);
					return;
				}
				else
				if (!_stricmp(cmd2, "del"))
				{
					if (4 <= argc)
					{
						int index = atoi(args->ArgV(3));

						for (std::list<CEntityBvhCapture *>::iterator it = m_EntityBvhCaptures.begin(); it != m_EntityBvhCaptures.end(); ++it)
						{
							if ((*it)->EntityIndex_get() == index)
							{
								delete (*it);

								m_EntityBvhCaptures.erase(it);
								return;
							}
						}

						Tier0_Warning("Error: Index %i not found!\n", index);
						return;
					}

					Tier0_Msg(
						"%s ent del <entityIndex> - Remove given <entityIndex> from list.\n"
						, prefix
					);
					return;
				}
				else
				if (!_stricmp(cmd2, "list"))
				{
					Tier0_Msg("<entityIndex>: <originType> <anglesType>\n");

					for (std::list<CEntityBvhCapture *>::iterator it = m_EntityBvhCaptures.begin(); it != m_EntityBvhCaptures.end(); ++it)
					{
						CEntityBvhCapture * cap = (*it);

						CEntityBvhCapture::Origin_e origin = cap->Origin_get();
						CEntityBvhCapture::Angles_e angles = cap->Angles_get();

						Tier0_Msg(
							"%i: %s %s"
							, cap->EntityIndex_get()
							, origin == CEntityBvhCapture::O_Net ? "net" : (origin == CEntityBvhCapture::O_View ? "view" : "[UNKNOWN]")
							, angles == CEntityBvhCapture::A_Net ? "net" : (angles == CEntityBvhCapture::A_View ? "view" : "[UNKNOWN]")
						);
					}

					Tier0_Msg("---- End Of List ----\n");
					return;
				}
				else
				if (!_stricmp(cmd2, "clear"))
				{
					while (!m_EntityBvhCaptures.empty())
					{
						delete m_EntityBvhCaptures.front();
						m_EntityBvhCaptures.pop_front();
					}

					Tier0_Msg("List cleared.\n");
					return;
				}
			}

			Tier0_Msg(
				"%s ent add [...] - Add an entity to the export list.\n"
				"%s ent del [...] - Remove an entity from the export list.\n"
				"%s ent list - List current entities being exported.\n"
				"%s ent clear - Remove all from list.\n"
				, prefix
				, prefix
				, prefix
				, prefix
			);
			return;
		}
	}

	Tier0_Msg(
		"%s cam [...] - Whether main camera export (overrides/uses mirv_camexport actually).\n"
		"%s ent [...] - Entity BVH export list control.\n"
		, prefix
		, prefix
	);
}

void CAfxStreams::Console_GameRecording(IWrpCommandArgs * args)
{
	int argc = args->ArgC();

	char const * prefix = args->ArgV(0);

	if (m_Recording)
	{
		Tier0_Warning("Error: These settings cannot be accessed during mirv_streams recording!\n");
		return;
	}

	CClientTools * clientTools = CClientTools::Instance();

	if (!clientTools)
	{
		Tier0_Warning("Error: Feature not available!\n");
		return;
	}

	if (2 <= argc)
	{
		char const * cmd1 = args->ArgV(1);

		if (!_stricmp(cmd1, "enabled"))
		{
			if (3 <= argc)
			{
				char const * cmd2 = args->ArgV(2);

				m_GameRecording = 0 != atoi(cmd2);
				return;
			}

			Tier0_Msg(
				"%s enabled 0|1 - Enable (1) / Disable (0) afxGameRecording (game state recording).\n"
				"Current value: %i.\n"
				, prefix
				, m_GameRecording ? 1 : 0
			);
			return;
		}
	}

	if (ClientTools_Console_Cfg(args))
		return;

	Tier0_Msg(
		"%s enabled [...]\n"
		, prefix
	);
}

SOURCESDK::IMaterialSystem_csgo * CAfxStreams::GetMaterialSystem(void)
{
	return m_MaterialSystem;
}

SOURCESDK::IShaderShadow_csgo * CAfxStreams::GetShaderShadow(void)
{
	return m_ShaderShadow;
}

std::wstring CAfxStreams::GetTakeDir(void)
{
	return m_TakeDir;
}

void CAfxStreams::LevelShutdown(IAfxBaseClientDll * cl)
{
	for(std::list<CAfxRecordStream *>::iterator it = m_Streams.begin(); it != m_Streams.end(); ++it)
	{
		(*it)->LevelShutdown();
	}
}

extern bool g_bD3D9DebugPrint;

void CAfxStreams::DebugDump(IAfxMatRenderContextOrg * ctxp)
{
	bool isRgba = true;

	int width = 1280;
	int height = 720;

	CAfxImageBuffer * buffer = ImageBufferPool.AquireBuffer();

	if(buffer->AutoRealloc(isRgba ? buffer->IBPF_BGRA : buffer->IBPF_BGR, width, height))
	{
		ctxp->ReadPixels(
			0, 0,
			width, height,
			(unsigned char*)buffer->Buffer,
			isRgba ? SOURCESDK::IMAGE_FORMAT_RGBA8888 : SOURCESDK::IMAGE_FORMAT_RGB888
		);

		// (back) transform to MDT native format:
		{
			int lastLine = height >> 1;
			if(height & 0x1) ++lastLine;

			for(int y=0;y<lastLine;++y)
			{
				int srcLine = y;
				int dstLine = height -1 -y;

				if(isRgba)
				{
					for(int x=0;x<width;++x)
					{
						unsigned char r = ((unsigned char *)buffer->Buffer)[dstLine*buffer->ImagePitch +4*x +0];
						unsigned char g = ((unsigned char *)buffer->Buffer)[dstLine*buffer->ImagePitch +4*x +1];
						unsigned char b = ((unsigned char *)buffer->Buffer)[dstLine*buffer->ImagePitch +4*x +2];
						unsigned char a = ((unsigned char *)buffer->Buffer)[dstLine*buffer->ImagePitch +4*x +3];
									
						((unsigned char *)buffer->Buffer)[dstLine*buffer->ImagePitch +4*x +0] = ((unsigned char *)buffer->Buffer)[srcLine*buffer->ImagePitch +4*x +2];
						((unsigned char *)buffer->Buffer)[dstLine*buffer->ImagePitch +4*x +1] = ((unsigned char *)buffer->Buffer)[srcLine*buffer->ImagePitch +4*x +1];
						((unsigned char *)buffer->Buffer)[dstLine*buffer->ImagePitch +4*x +2] = ((unsigned char *)buffer->Buffer)[srcLine*buffer->ImagePitch +4*x +0];
						((unsigned char *)buffer->Buffer)[dstLine*buffer->ImagePitch +4*x +3] = ((unsigned char *)buffer->Buffer)[srcLine*buffer->ImagePitch +4*x +3];

						((unsigned char *)buffer->Buffer)[srcLine*buffer->ImagePitch +4*x +0] = b;
						((unsigned char *)buffer->Buffer)[srcLine*buffer->ImagePitch +4*x +1] = g;
						((unsigned char *)buffer->Buffer)[srcLine*buffer->ImagePitch +4*x +2] = r;
						((unsigned char *)buffer->Buffer)[srcLine*buffer->ImagePitch +4*x +3] = a;
					}
				}
				else
				{
					for(int x=0;x<width;++x)
					{
						unsigned char r = ((unsigned char *)buffer->Buffer)[dstLine*buffer->ImagePitch +3*x +0];
						unsigned char g = ((unsigned char *)buffer->Buffer)[dstLine*buffer->ImagePitch +3*x +1];
						unsigned char b = ((unsigned char *)buffer->Buffer)[dstLine*buffer->ImagePitch +3*x +2];
									
						((unsigned char *)buffer->Buffer)[dstLine*buffer->ImagePitch +3*x +0] = ((unsigned char *)buffer->Buffer)[srcLine*buffer->ImagePitch +3*x +2];
						((unsigned char *)buffer->Buffer)[dstLine*buffer->ImagePitch +3*x +1] = ((unsigned char *)buffer->Buffer)[srcLine*buffer->ImagePitch +3*x +1];
						((unsigned char *)buffer->Buffer)[dstLine*buffer->ImagePitch +3*x +2] = ((unsigned char *)buffer->Buffer)[srcLine*buffer->ImagePitch +3*x +0];
									
						((unsigned char *)buffer->Buffer)[srcLine*buffer->ImagePitch +3*x +0] = b;
						((unsigned char *)buffer->Buffer)[srcLine*buffer->ImagePitch +3*x +1] = g;
						((unsigned char *)buffer->Buffer)[srcLine*buffer->ImagePitch +3*x +2] = r;
					}
				}
			}
		}

		// Write to disk:
		{
			std::wstring path = L"debug.tga";
			if(!buffer->WriteToFile(path, false, false))
			{
				Tier0_Warning("CAfxStreams::DebugDump:Failed writing image for frame #%i\n.", m_Frame);
			}
		}
	}
	else
	{
		Tier0_Warning("CAfxStreams::DebugDump: Failed to realloc m_BufferA.\n");
	}

}

void CAfxStreams::View_Render(IAfxBaseClientDll * cl, SOURCESDK::vrect_t_csgo *rect)
{
	CAfxBaseFxStream::MainThreadInitialize();
	
	IAfxMatRenderContextOrg * ctxp = GetCurrentContext()->GetOrg();

	BlockPresent(ctxp, false);
	m_PresentBlocked = false;

	SetCurrent_View_Render_ThreadId(GetCurrentThreadId());

	bool canFeed = CheckCanFeedStreams();

	CAfxRenderViewStream * previewStream = 0;
	if(m_PreviewStream)
	{
		if(CAfxSingleStream * singleStream = m_PreviewStream->AsAfxSingleStream())
		{
			previewStream = singleStream->Stream_get();
		}
	}

	bool previewStreamWillRecord = false;

	if(previewStream)
	{
		if(!canFeed)
			Tier0_Warning("Error: Cannot preview stream %s due to missing dependencies!\n", m_PreviewStream->StreamName_get());
		else
		{
			SetMatVarsForStreams(); // keep them set in case a mofo resets them.

			if (0 < strlen(previewStream->AttachCommands_get()))
				g_VEngineClient->ExecuteClientCmd(previewStream->AttachCommands_get()); // Execute commands before we lock the stream!

			previewStream->OnRenderBegin();

			ctxp->ClearColor4ub(0,0,0,0);
			ctxp->ClearBuffers(true,false,false);
		}
	}

	//GetCsgoCGlowOverlayFix()->OnMainViewRenderBegin();

	cl->GetParent()->View_Render(rect);

#ifdef AFX_MIRV_PGL
	if (MirvPgl::IsDataActive())
	{
		MirvPgl::CamData camData(
			g_Hook_VClient_RenderView.GetCurTime(),
			(float)g_Hook_VClient_RenderView.LastCameraOrigin[0],
			(float)g_Hook_VClient_RenderView.LastCameraOrigin[1],
			(float)g_Hook_VClient_RenderView.LastCameraOrigin[2],
			(float)g_Hook_VClient_RenderView.LastCameraAngles[2],
			(float)g_Hook_VClient_RenderView.LastCameraAngles[0],
			(float)g_Hook_VClient_RenderView.LastCameraAngles[1],
			(float)(AlienSwarm_FovScaling(rect->width, rect->height, g_Hook_VClient_RenderView.LastCameraFov))
		);

		QueueOrExecute(ctxp, new CAfxLeafExecute_Functor(new AfxSupplyCamData_Functor(camData)));
	}
#endif

	// Capture
	if (m_CamExportObj)
	{
		CamIO::CamData camData;

		camData.Time = g_Hook_VClient_RenderView.GetCurTime();
		camData.XPosition = g_Hook_VClient_RenderView.LastCameraOrigin[0];
		camData.YPosition = g_Hook_VClient_RenderView.LastCameraOrigin[1];
		camData.ZPosition = g_Hook_VClient_RenderView.LastCameraOrigin[2];
		camData.YRotation = g_Hook_VClient_RenderView.LastCameraAngles[0];
		camData.ZRotation = g_Hook_VClient_RenderView.LastCameraAngles[1];
		camData.XRotation = g_Hook_VClient_RenderView.LastCameraAngles[2];
		camData.Fov = g_Hook_VClient_RenderView.LastCameraFov;

		m_CamExportObj->WriteFrame(g_Hook_VClient_RenderView.LastWidth, g_Hook_VClient_RenderView.LastHeight, camData);
	}

	// Capture BVHs (except main):
	for (std::list<CEntityBvhCapture *>::iterator it = m_EntityBvhCaptures.begin(); it != m_EntityBvhCaptures.end(); ++it)
	{
		(*it)->CaptureFrame();
	}

	if(previewStream && canFeed)
	{
		previewStreamWillRecord = m_Recording && canFeed && m_PreviewStream->Record_get();

		previewStream->OnRenderEnd();

		if (0 < strlen(previewStream->DetachCommands_get()))
			g_VEngineClient->ExecuteClientCmd(previewStream->DetachCommands_get()); // Execute commands after we unlocked the stream!
	}

	if(m_Recording)
	{
		if(!canFeed)
		{
			Tier0_Warning("Error: Cannot record streams due to missing dependencies!\n");
		}
		else
		{
			for(std::list<CAfxRecordStream *>::iterator it = m_Streams.begin(); it != m_Streams.end(); ++it)
			{
				if(!(*it)->Record_get()) continue;

				//
				// Render a stream and queue capturing etc.:

				CAfxRenderViewStream * streamA = 0;
				CAfxRenderViewStream * streamB = 0;
				bool streamAOk = false;
				bool streamBOk = false;

				CAfxSingleStream * curSingle = (*it)->AsAfxSingleStream();
				CAfxTwinStream * curTwin = (*it)->AsAfxTwinStream();

				if(curSingle)
				{
					streamA = curSingle->Stream_get();
				}
				else
				if(curTwin)
				{
					CAfxTwinStream::StreamCombineType streamCombineType = curTwin->StreamCombineType_get();

					if(CAfxTwinStream::SCT_ARedAsAlphaBColor == streamCombineType)
					{
						streamA = curTwin->StreamB_get();
						streamB = curTwin->StreamA_get();
					}
					else
					if(CAfxTwinStream::SCT_AColorBRedAsAlpha == streamCombineType)
					{
						streamA = curTwin->StreamA_get();
						streamB = curTwin->StreamB_get();
					}
				}

				if(streamA)
				{
					ctxp = CaptureStreamToBuffer(streamA, (*it), streamA == previewStream && previewStreamWillRecord, true, !streamB);
				}

				if(streamB)
				{
					ctxp = CaptureStreamToBuffer(streamB, (*it), streamB == previewStream && previewStreamWillRecord, !streamA, true);
				}
			}
		}

		++m_Frame;
	}

	SetCurrent_View_Render_ThreadId(0);
}

IAfxMatRenderContextOrg * CAfxStreams::CaptureStreamToBuffer(CAfxRenderViewStream * stream, CAfxRecordStream * captureTarget, bool isInPreview, bool first, bool last)
{
	if (!m_PresentRecordOnScreen)
	{
		m_MaterialSystem->SwapBuffers();
	}

	// Work around game running out of memory because of too much shit on the queue
	// aka issue ripieces/advancedfx#22 :
	m_MaterialSystem->EndFrame();
	m_MaterialSystem->BeginFrame(0);

	//
	// We are on potentially a new context now!

	IAfxMatRenderContextOrg * ctxp = GetCurrentContext()->GetOrg();

	if (first)
	{
		captureTarget->QueueCaptureStart(ctxp);
	}

	if (!m_PresentRecordOnScreen)
	{
		if (!m_PresentBlocked)
		{
			BlockPresent(ctxp, true);
			m_PresentBlocked = true;
		}
	}
	
	CAfxRenderViewStream::StreamCaptureType captureType = stream->StreamCaptureType_get();
	bool isDepthF = captureType == CAfxRenderViewStream::SCT_DepthF || captureType == CAfxRenderViewStream::SCT_DepthFZIP;

	SOURCESDK::IViewRender_csgo * view = GetView_csgo();

	const SOURCESDK::CViewSetup_csgo * viewSetup = view->GetViewSetup();

	if(isDepthF)
	{
		if(m_RenderTargetDepthF)
		{
			ctxp->PushRenderTargetAndViewport(
				m_RenderTargetDepthF,
				0,
				viewSetup->m_nUnscaledX,
				viewSetup->m_nUnscaledY,
				viewSetup->m_nUnscaledWidth,
				viewSetup->m_nUnscaledHeight
			);
		}
		else
		{
			Tier0_Warning("AFXERROR: CAfxStreams::CaptureStreamToBuffer: missing render target.\n");
			return false;
		}
	}
	else
	{
		//if(m_PresentRecordOnScreen)
		//	m_MaterialSystem->SwapBuffers();
	}

	SetMatVarsForStreams(); // keep them set in case a mofo resets them.

	if (0 < strlen(stream->AttachCommands_get()))
		g_VEngineClient->ExecuteClientCmd(stream->AttachCommands_get()); // Execute commands before we lock the stream!

	stream->OnRenderBegin();

	int whatToDraw = SOURCESDK::RENDERVIEW_UNSPECIFIED;

	if(stream->DrawHud_get()) whatToDraw |= SOURCESDK::RENDERVIEW_DRAWHUD;
	if(stream->DrawViewModel_get()) whatToDraw |= SOURCESDK::RENDERVIEW_DRAWVIEWMODEL;

	ctxp->ClearColor4ub(0,0,0,0);
	ctxp->ClearBuffers(true,false,false);

	//DrawLock(ctxp);
	//GetCsgoCGlowOverlayFix()->OnStreamRenderViewBegin(stream);
	view->RenderView(*viewSetup, *viewSetup, SOURCESDK::VIEW_CLEAR_STENCIL|SOURCESDK::VIEW_CLEAR_DEPTH, whatToDraw);
	//ScheduleDrawUnlock(ctxp);

	stream->QueueCapture(ctxp, captureTarget,
		viewSetup->m_nUnscaledX,
		viewSetup->m_nUnscaledY,
		viewSetup->m_nUnscaledWidth,
		viewSetup->m_nUnscaledHeight
		);

	stream->OnRenderEnd();

	if (0 < strlen(stream->DetachCommands_get()))
		g_VEngineClient->ExecuteClientCmd(stream->DetachCommands_get()); // Execute commands after we lock the stream!

	if(isDepthF)
	{
		ctxp->PopRenderTargetAndViewport();
	}

	if (last)
	{
		captureTarget->QueueCaptureEnd(ctxp, m_TakeDir, m_Frame, (captureType == CAfxRenderViewStream::SCT_Depth24 || captureType == CAfxRenderViewStream::SCT_Depth24ZIP || captureType == CAfxRenderViewStream::SCT_DepthF || captureType == CAfxRenderViewStream::SCT_DepthFZIP) ? L".exr" : (m_FormatBmpAndNotTga ? L".bmp" : L".tga"));
	}

	return ctxp;
}

bool CAfxStreams::Console_CheckStreamName(char const * value)
{
	if(StringIsEmpty(value))
	{
		Tier0_Msg("Error: Stream name can not be emty.\n");
		return false;
	}
	if(!StringIsAlNum(value))
	{
		Tier0_Msg("Error: Stream name must be alphanumeric.\n");
		return false;
	}

	// Check if name is unique:
	{
		int index = 0;
		for(std::list<CAfxRecordStream *>::iterator it = m_Streams.begin(); it != m_Streams.end(); ++it)
		{
			if(!_stricmp((*it)->StreamName_get(), value))
			{
				Tier0_Msg("Error: Stream name must be unique, \"%s\" is already in use by stream with index %i.\n", value, index);
				return false;
			}

			++index;
		}
	}

	return true;
}

bool CAfxStreams::Console_ToAfxAction(char const * value, CAfxBaseFxStream::CAction * & action)
{
	CAfxBaseFxStream::CAction * tmpAction = CAfxBaseFxStream::GetAction(CAfxBaseFxStream::CActionKey(value));

	if(tmpAction)
	{
		action = tmpAction;
		return true;
	}

	Tier0_Warning("Invalid action name.\n");
	return false;
}

char const * CAfxStreams::Console_FromAfxAction(CAfxBaseFxStream::CAction * action)
{
	if(action)
	{
		return action->Key_get().m_Name.c_str();
	}

	return "[null]";
}

bool CAfxStreams::Console_ToStreamCombineType(char const * value, CAfxTwinStream::StreamCombineType & streamCombineType)
{
	if(!_stricmp(value, "aRedAsAlphaBColor"))
	{
		streamCombineType = CAfxTwinStream::SCT_ARedAsAlphaBColor;
		return true;
	}
	else
	if(!_stricmp(value, "aColorBRedAsAlpha"))
	{
		streamCombineType = CAfxTwinStream::SCT_AColorBRedAsAlpha;
		return true;
	}

	return false;
}

char const * CAfxStreams::Console_FromStreamCombineType(CAfxTwinStream::StreamCombineType streamCombineType)
{
	switch(streamCombineType)
	{
	case CAfxTwinStream::SCT_ARedAsAlphaBColor:
		return "aRedAsAlphaBColor";
	case CAfxTwinStream::SCT_AColorBRedAsAlpha:
		return "aColorBRedAsAlpha";
	}

	return "[unkown]";
}

bool CAfxStreams::Console_ToStreamCaptureType(char const * value, CAfxRenderViewStream::StreamCaptureType & StreamCaptureType)
{
	if(!_stricmp(value, "normal"))
	{
		StreamCaptureType = CAfxRenderViewStream::SCT_Normal;
		return true;
	}
	else
	if(!_stricmp(value, "depth24"))
	{
		StreamCaptureType = CAfxRenderViewStream::SCT_Depth24;
		return true;
	}
	else
	if(!_stricmp(value, "depth24ZIP"))
	{
		StreamCaptureType = CAfxRenderViewStream::SCT_Depth24ZIP;
		return true;
	}
	else
	if(!_stricmp(value, "depthF"))
	{
		StreamCaptureType = CAfxRenderViewStream::SCT_DepthF;
		return true;
	}
	else
	if(!_stricmp(value, "depthFZIP"))
	{
		StreamCaptureType = CAfxRenderViewStream::SCT_DepthFZIP;
		return true;
	}

	return false;
}

char const * CAfxStreams::Console_FromStreamCaptureType(CAfxRenderViewStream::StreamCaptureType StreamCaptureType)
{
	switch(StreamCaptureType)
	{
	case CAfxRenderViewStream::SCT_Normal:
		return "nomral";
	case CAfxRenderViewStream::SCT_Depth24:
		return "depth24";
	case CAfxRenderViewStream::SCT_Depth24ZIP:
		return "depth24ZIP";
	case CAfxRenderViewStream::SCT_DepthF:
		return "depthF";
	case CAfxRenderViewStream::SCT_DepthFZIP:
		return "depthFZIP";
	}

	return "[unkown]";

}

bool CAfxStreams::CheckCanFeedStreams(void)
{
	return 0 != GetView_csgo()
		&& 0 != m_MaterialSystem
		&& 0 != m_AfxBaseClientDll
		&& 0 != m_ShaderShadow
	;
}

void CAfxStreams::BackUpMatVars()
{
	EnsureMatVars();

	m_OldMatPostProcessEnable = m_MatPostProcessEnableRef->GetInt();
	m_OldMatDynamicTonemapping = m_MatDynamicTonemappingRef->GetInt();
	m_OldMatMotionBlurEnabled = m_MatMotionBlurEnabledRef->GetInt();
	m_OldMatForceTonemapScale = m_MatForceTonemapScale->GetFloat();
	m_OldSndMuteLosefocus = m_SndMuteLosefocus->GetInt();
	m_OldSndMixAsync = m_SndMixAsync->GetInt();
}

void CAfxStreams::SetMatVarsForStreams()
{
	EnsureMatVars();

	m_MatPostProcessEnableRef->SetValue((float)m_NewMatPostProcessEnable);
	m_MatDynamicTonemappingRef->SetValue((float)m_NewMatDynamicTonemapping);
	m_MatMotionBlurEnabledRef->SetValue((float)m_NewMatMotionBlurEnabled);
	m_MatForceTonemapScale->SetValue(m_NewMatForceTonemapScale);
	m_SndMuteLosefocus->SetValue(0.0f);
	m_SndMixAsync->SetValue(0.0f);
}

void CAfxStreams::RestoreMatVars()
{
	EnsureMatVars();

	m_MatPostProcessEnableRef->SetValue((float)m_OldMatPostProcessEnable);
	m_MatDynamicTonemappingRef->SetValue((float)m_OldMatDynamicTonemapping);
	m_MatMotionBlurEnabledRef->SetValue((float)m_OldMatMotionBlurEnabled);
	m_MatForceTonemapScale->SetValue(m_OldMatForceTonemapScale);
	m_SndMuteLosefocus->SetValue((float)m_OldSndMuteLosefocus);
	m_SndMixAsync->SetValue((float)m_OldSndMixAsync);
}

void CAfxStreams::EnsureMatVars()
{
	if(!m_MatPostProcessEnableRef) m_MatPostProcessEnableRef = new WrpConVarRef("mat_postprocess_enable");
	if(!m_MatDynamicTonemappingRef) m_MatDynamicTonemappingRef = new WrpConVarRef("mat_dynamic_tonemapping");
	if(!m_MatMotionBlurEnabledRef) m_MatMotionBlurEnabledRef = new WrpConVarRef("mat_motion_blur_enabled");
	if(!m_MatForceTonemapScale) m_MatForceTonemapScale = new WrpConVarRef("mat_force_tonemap_scale");
	if (!m_SndMuteLosefocus) m_SndMuteLosefocus = new WrpConVarRef("snd_mute_losefocus");
	if (!m_SndMixAsync) m_SndMixAsync = new WrpConVarRef("snd_mix_async");
}

void CAfxStreams::AddStream(CAfxRecordStream * stream)
{
	stream->AddRef();
	m_Streams.push_back(stream);

	if(m_Recording) stream->RecordStart();
}

void CAfxStreams::CreateRenderTargets(SOURCESDK::IMaterialSystem_csgo * materialSystem)
{
	materialSystem->BeginRenderTargetAllocation();

/*
	m_RgbaRenderTarget = materialSystem->CreateRenderTargetTexture(0,0,RT_SIZE_FULL_FRAME_BUFFER,IMAGE_FORMAT_RGBA8888);
	if(m_RgbaRenderTarget)
	{
		m_RgbaRenderTarget->IncrementReferenceCount();
	}
	else
	{
		Tier0_Warning("AFXERROR: CAfxStreams::CreateRenderTargets no m_RgbaRenderTarget (affects rgba captures)!\n");
	}
*/

	m_RenderTargetDepthF = materialSystem->CreateRenderTargetTexture(0,0, SOURCESDK::RT_SIZE_FULL_FRAME_BUFFER, SOURCESDK::IMAGE_FORMAT_R32F, SOURCESDK::MATERIAL_RT_DEPTH_SHARED);
	if(m_RenderTargetDepthF)
	{
		m_RenderTargetDepthF->IncrementReferenceCount();
	}
	else
	{
		Tier0_Warning("AFXWARNING: CAfxStreams::CreateRenderTargets no m_RenderTargetDepthF (affects high precision depthF captures)!\n");
	}

	materialSystem->EndRenderTargetAllocation();

}

IAfxContextHook * CAfxStreams::FindHook(IAfxMatRenderContext * ctx)
{
	if (!ctx)
		return 0;

	return ctx->Hook_get();
}

// CAfxImageBufferPool /////////////////////////////////////////////////////////

CAfxImageBufferPool::CAfxImageBufferPool()
{
	for (int i = 0; i < 3; ++i)
	{
		m_Buffers.push(new CAfxImageBuffer(this));
	}

	m_BufferAvailableCondition.notify_one();
}

CAfxImageBufferPool::~CAfxImageBufferPool()
{
	while (!m_Buffers.empty())
	{
		delete m_Buffers.top();
		m_Buffers.pop();
	}
}

CAfxImageBuffer * CAfxImageBufferPool::AquireBuffer(void)
{
	std::unique_lock<std::mutex> lock(m_BuffersMutex);

	m_BufferAvailableCondition.wait(lock, [this]() { return !m_Buffers.empty(); });

	CAfxImageBuffer * result = m_Buffers.top();

	m_Buffers.pop();

	return result;
}

void CAfxImageBufferPool::ImageBuffer_Done(CAfxImageBuffer * buffer)
{
	{
		std::unique_lock<std::mutex> lock(m_BuffersMutex);

		m_Buffers.push(buffer);
	}

	m_BufferAvailableCondition.notify_one();
}

// CAfxImageBuffer /////////////////////////////////////////////////////////////

CAfxImageBuffer::CAfxImageBuffer(CAfxImageBufferPool * pool)
: Buffer(0)
, m_BufferBytesAllocated(0)
, m_Pool(pool)
{
}

CAfxImageBuffer::~CAfxImageBuffer()
{
	free(Buffer);
}

void CAfxImageBuffer::Release(void)
{
	m_Pool->ImageBuffer_Done(this);
}

bool CAfxImageBuffer::AutoRealloc(ImageBufferPixelFormat pixelFormat, int width, int height)
{
	size_t pitch = width;

	switch(pixelFormat)
	{
	case IBPF_BGR:
		pitch *= 3 * sizeof(char);
		break;
	case IBPF_BGRA:
		pitch *= 4 * sizeof(char);
		break;
	case IBPF_A:
		pitch *= 1 * sizeof(char);
		break;
	case IBPF_ZFloat:
		pitch *= 1 * sizeof(float);
		break;
	default:
		Tier0_Warning("CAfxImageBuffer::AutoRealloc: Unsupported pixelFormat\n");
		return false;
	}

	size_t imageBytes = pitch * height;

	if( !Buffer || m_BufferBytesAllocated < imageBytes)
	{
		Buffer = realloc(Buffer, imageBytes);
		if(Buffer)
		{
			m_BufferBytesAllocated = imageBytes;
		}
	}

	m_BufferBytesAllocated = imageBytes;
	PixelFormat = pixelFormat;
	Width = width;
	Height = height;
	ImagePitch = pitch;
	ImageBytes = imageBytes;

	return 0 != Buffer;
}


bool CAfxImageBuffer::WriteToFile(const std::wstring & path, bool ifZip, bool ifBmpNotTga) const
{
	if (IBPF_ZFloat == PixelFormat)
	{
		return WriteFloatZOpenExr(
			path.c_str(),
			(unsigned char*)Buffer,
			Width,
			Height,
			sizeof(float),
			ImagePitch,
			ifZip ? WFZOEC_Zip : WFZOEC_None
		);
	}

	if (IBPF_A == PixelFormat)
	{
		return ifBmpNotTga
			? WriteRawBitmap((unsigned char*)Buffer, path.c_str(), Width, Height, 8, ImagePitch)
			: WriteRawTarga((unsigned char*)Buffer, path.c_str(), Width, Height, 8, true, ImagePitch, 0)
			;
	}

	bool isBgra = IBPF_BGRA == PixelFormat;

	return ifBmpNotTga && !isBgra
		? WriteRawBitmap((unsigned char*)Buffer, path.c_str(), Width, Height, 24, ImagePitch)
		: WriteRawTarga((unsigned char*)Buffer, path.c_str(), Width, Height, isBgra ? 32 : 24, false, ImagePitch, isBgra ? 8 : 0)
		;
}

bool CAfxImageBuffer::BgrMergeBlueToRgba(CAfxImageBuffer const * alphaBuffer)
{
	bool ok = alphaBuffer
		&& Width == alphaBuffer->Width
		&& Height == alphaBuffer->Height
		&& PixelFormat == IBPF_BGR
		&& PixelFormat == alphaBuffer->PixelFormat
		&& ImagePitch == alphaBuffer->ImagePitch
		&& AutoRealloc(IBPF_BGRA, Width, Height)
		;

	if (ok)
	{
		// interleave B as alpha into A:

		for (int y = Height - 1;y >= 0;--y)
		{
			for (int x = Width - 1;x >= 0;--x)
			{
				unsigned char b = ((unsigned char *)Buffer)[y*ImagePitch + x * 3 + 0];
				unsigned char g = ((unsigned char *)Buffer)[y*ImagePitch + x * 3 + 1];
				unsigned char r = ((unsigned char *)Buffer)[y*ImagePitch + x * 3 + 2];
				unsigned char a = ((unsigned char *)alphaBuffer->Buffer)[y*alphaBuffer->ImagePitch + x * 3 + 0];

				((unsigned char *)Buffer)[y*ImagePitch + x * 4 + 0] = b;
				((unsigned char *)Buffer)[y*ImagePitch + x * 4 + 1] = g;
				((unsigned char *)Buffer)[y*ImagePitch + x * 4 + 2] = r;
				((unsigned char *)Buffer)[y*ImagePitch + x * 4 + 3] = a;
			}
		}
	}
	else
	{
		Tier0_Warning("CAfxStreams::View_Render: Combining streams failed.\n");
	}

	return ok;
}

// CAfxStreams::CEntityBvhCapture //////////////////////////////////////////////

CAfxStreams::CEntityBvhCapture::CEntityBvhCapture(int entityIndex, Origin_e origin, Angles_e angles)
: m_BvhExport(0)
, m_EntityIndex(entityIndex)
, m_Origin(origin)
, m_Angles(angles)
{
}

CAfxStreams::CEntityBvhCapture::~CEntityBvhCapture()
{
	EndCapture();
}

void CAfxStreams::CEntityBvhCapture::StartCapture(std::wstring const & takePath, double frameTime)
{
	EndCapture();

	std::wostringstream os;
	os << takePath << L"\\cam_ent_" << m_EntityIndex << L".bvh";

	m_BvhExport = new BvhExport(
		os.str().c_str(),
		"MdtCam",
		frameTime
	);
}

void CAfxStreams::CEntityBvhCapture::EndCapture(void)
{
	if (!m_BvhExport)
		return;

	delete m_BvhExport;
	m_BvhExport = 0;
}

void CAfxStreams::CEntityBvhCapture::CaptureFrame(void)
{
	if (!m_BvhExport)
		return;

	SOURCESDK::Vector o;
	SOURCESDK::QAngle a;

	SOURCESDK::IClientEntity_csgo * ce = SOURCESDK::g_Entitylist_csgo->GetClientEntity(m_EntityIndex);
	SOURCESDK::C_BaseEntity_csgo * be = ce ? ce->GetBaseEntity() : 0;

	if (ce)
	{
		o = be && O_View == m_Origin ? be->EyePosition() : ce->GetAbsOrigin();
		a = be && A_View == m_Angles ? be->EyeAngles() : ce->GetAbsAngles();
	}
	else
	{
		o.x = 0;
		o.y = 0;
		o.z = 0;

		a.x = 0;
		a.y = 0;
		a.z = 0;
	}

	m_BvhExport->WriteFrame(
		-o.y, +o.z, -o.x,
		-a.z, -a.x, +a.y
	);
}

void CAfxStreams::BlockPresent(IAfxMatRenderContextOrg * ctx, bool value)
{
	QueueOrExecute(ctx, new CAfxLeafExecute_Functor(new AfxD3D9BlockPresent_Functor(value)));
}

void CAfxStreams::AfxStreamsInit(void)
{
	CAfxBaseFxStream::AfxStreamsInit();
}

void CAfxStreams::ShutDown(void)
{
	if (!m_ShutDown)
	{
		m_ShutDown = true;
		
		CAfxBaseFxStream::AfxStreamsShutdown();

		delete m_CamExportObj;

		while (!m_EntityBvhCaptures.empty())
		{
			delete m_EntityBvhCaptures.front();
			m_EntityBvhCaptures.pop_front();
		}

		while (!m_Streams.empty())
		{
			m_Streams.front()->Release();
			m_Streams.pop_front();
		}

		delete m_MatPostProcessEnableRef;
		delete m_HostFrameRate;
	}
}
