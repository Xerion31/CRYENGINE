// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

/********************************************************************
   -------------------------------------------------------------------------
   File name:   FlowNodeAIAction.h
   $Id$
   Description: place for miscellaneous AI related flow graph nodes

   -------------------------------------------------------------------------
   History:
   - 15:6:2005   15:24 : Created by Kirill Bulatsev

 *********************************************************************/

#ifndef __FlowNodeAIAction_H__
#define __FlowNodeAIAction_H__
#pragma once

#include "FlowBaseNode.h"
#include "Vehicle/FlowVehicleBase.h"
#include "IAnimationGraph.h"

#include "Vehicle/FlowVehicleBase.h"

//////////////////////////////////////////////////////////////////////////
// base AI Flow node
//////////////////////////////////////////////////////////////////////////
template<bool TBlocking>
class CFlowNode_AIBase : public CFlowBaseNode<eNCT_Instanced>, public IEntityEventListener, public IGoalPipeListener
{
public:
	static const bool m_bBlocking = TBlocking;

	CFlowNode_AIBase(SActivationInfo* pActInfo);
	virtual ~CFlowNode_AIBase();

	// Derived classes must re-implement this method!!!
	virtual void GetConfiguration(SFlowNodeConfig& config) = 0;

	// Derived classes may re-implement this method!!!
	// Default implementation creates new instance for each cloned node
	// must be replicated to avoid derived template argument for this class
	virtual IFlowNodePtr Clone(SActivationInfo* pActInfo) = 0;
	virtual void         Serialize(SActivationInfo*, TSerialize ser);
	// Derived classes normally shouldn't override this method!!!
	virtual void         ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo);
	// Derived classes must implement this method!!!
	virtual void         DoProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) = 0;
	// Derived classes may override this method to be notified when the action was restored
	virtual void         OnResume(SActivationInfo* pActInfo = NULL) {}
	// Derived classes may override this method to be notified when the action was canceled
	virtual void         OnCancel()                                 {}
	virtual void         OnFinish()                                 {};
	// Derived classes may override this method to be updated each frame
	virtual bool         OnUpdate(SActivationInfo* pActInfo)        { return false; }

	//////////////////////////////////////////////////////////////////////////
	// IGoalPipeListener
	virtual void OnGoalPipeEvent(IPipeUser* pPipeUser, EGoalPipeEvent event, int goalPipeId, bool& unregisterListenerAfterEvent);

	//////////////////////////////////////////////////////////////////////////
	// IEntityEventListener
	virtual void OnEntityEvent(IEntity* pEntity, SEntityEvent& event);

	void         RegisterEvents();
	virtual void UnregisterEvents();

	IEntity*     GetEntity(SActivationInfo* pActInfo);

	bool         Execute(SActivationInfo* pActInfo, const char* pSignalText, IAISignalExtraData* pData = NULL, int senderId = 0);

protected:
	virtual void OnCancelPortActivated(IPipeUser* pPipeUser, SActivationInfo* pActInfo);

	virtual void Cancel();
	virtual void Finish();

	void         RegisterEntityEvents();
	void         RegisterAIEvents();

	//TODO: should not store this - must be read from inPort
	IFlowGraph* m_pGraph;
	TFlowNodeId m_nodeID;

	int         m_GoalPipeId;
	int         m_UnregisterGoalPipeId;
	EntityId    m_EntityId;
	EntityId    m_UnregisterEntityId;
	bool        m_bExecuted;
	bool        m_bSynchronized;
	bool        m_bNeedsExec;
	bool        m_bNeedsSink;
	bool        m_bNeedsReset;

	// you might want to override this method
	virtual bool ExecuteOnAI(SActivationInfo* pActInfo, const char* pSignalText,
	                         IAISignalExtraData* pData, IEntity* pEntity, IEntity* pSender);

	// you might want to override this method
	virtual bool ExecuteOnEntity(SActivationInfo* pActInfo, const char* pSignalText,
	                             IAISignalExtraData* pData, IEntity* pEntity, IEntity* pSender);

	// Utility function to set an AI's speed.
	void SetSpeed(IAIObject* pAI, int iSpeed);

	// Utility function to set an AI's stance.
	void SetStance(IAIObject* pAI, int stance);

	// Utility function to set an AI's a.
	void SetAllowStrafing(IAIObject* pAI, bool bAllowStrafing);

	// should call DoProcessEvent if owner is not too much alerted
	virtual void TryExecute(IAIObject* pAI, IFlowNode::EFlowEvent event, IFlowNode::SActivationInfo* pActInfo);

	// this will be called when the node is activated, one update before calling ExecuteIfNotTooMuchAlerted
	// use this if there's some data that needs to be initialized before execution
	virtual void PreExecute(SActivationInfo* pActInfo) {}
};

//---------------------------------------------------------------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////
// base AI forceable
//////////////////////////////////////////////////////////////////////////
template<bool TBlocking>
class CFlowNode_AIForceableBase
	: public CFlowNode_AIBase<TBlocking>
{
	typedef CFlowNode_AIBase<TBlocking> TBase;
public:
	CFlowNode_AIForceableBase(IFlowNode::SActivationInfo* pActInfo)
		: CFlowNode_AIBase<TBlocking>(pActInfo)
		, m_LastForceMethod(eNoForce) {}

	virtual void ProcessEvent(IFlowNode::EFlowEvent event, IFlowNode::SActivationInfo* pActInfo);
	virtual void Serialize(IFlowNode::SActivationInfo*, TSerialize ser);

protected:
	enum EForceMethod
	{
		eNoForce = 0,
		eKeepPerception,
		eIgnoreAll,
	};

	virtual void         OnCancel();
	virtual void         OnFinish();
	virtual void         TryExecute(IAIObject* pAI, IFlowNode::EFlowEvent event, IFlowNode::SActivationInfo* pActInfo);
	virtual EForceMethod GetForceMethod(IFlowNode::SActivationInfo* pActInfo) const
	{
		assert(!"Must implement for derived classes!");
		return eNoForce;
	}
	virtual void SetForceMethod(IAIObject* pAI, EForceMethod method);

	EForceMethod m_LastForceMethod;
};

//---------------------------------------------------------------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////
// base AI Signal order
//////////////////////////////////////////////////////////////////////////
template<class TDerivedFromSignalBase>
class CFlowNode_AISignalBase
	: public CFlowNode_AIBase<false>
{
	typedef CFlowNode_AIBase<false> TBase;
public:
	CFlowNode_AISignalBase(IFlowNode::SActivationInfo* pActInfo)
		: CFlowNode_AIBase<false>(pActInfo)
		, m_SignalText(nullptr)
	{
	}

	virtual void         GetConfiguration(SFlowNodeConfig& config);
	virtual void         DoProcessEvent(IFlowNode::EFlowEvent event, IFlowNode::SActivationInfo* pActInfo);
	virtual IFlowNodePtr Clone(SActivationInfo* pActInfo);

protected:
	virtual void Cancel();
	virtual void Finish();

	virtual void TryExecute(IAIObject* pAI, IFlowNode::EFlowEvent event, IFlowNode::SActivationInfo* pActInfo);
	virtual bool ShouldForce(IFlowNode::SActivationInfo* pActInfo) const;

	const char* m_SignalText;

	virtual IAISignalExtraData* GetExtraData(IFlowNode::SActivationInfo* pActInfo) const { return NULL; }
};

//---------------------------------------------------------------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////
// base AI Signal order
//////////////////////////////////////////////////////////////////////////
class CFlowNode_AISignalAlerted
	: public CFlowNode_AISignalBase<CFlowNode_AISignalAlerted>
{
public:
	CFlowNode_AISignalAlerted(IFlowNode::SActivationInfo* pActInfo)
		: CFlowNode_AISignalBase<CFlowNode_AISignalAlerted>(pActInfo) { m_SignalText = "ACT_ALERTED"; }

	virtual void GetMemoryUsage(ICrySizer* s) const
	{
		s->Add(*this);
	}
};

//////////////////////////////////////////////////////////////////////////
// prototyping AI orders
//////////////////////////////////////////////////////////////////////////
//---------------------------------------------------------------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////
// generic AI order
//////////////////////////////////////////////////////////////////////////
class CFlowNode_AISignal
	: public CFlowNode_AISignalBase<CFlowNode_AISignal>
{
	typedef CFlowNode_AISignalBase<CFlowNode_AISignal> TBase;
public:
	CFlowNode_AISignal(IFlowNode::SActivationInfo* pActInfo) : TBase(pActInfo) {}
	virtual void         GetConfiguration(SFlowNodeConfig& config);
	virtual void         DoProcessEvent(IFlowNode::EFlowEvent event, IFlowNode::SActivationInfo* pActInfo);
	virtual IFlowNodePtr Clone(SActivationInfo* pActInfo);

	virtual void         OnCancel();
	virtual void         OnFinish();

	virtual bool         ShouldForce(IFlowNode::SActivationInfo* pActInfo) const;

	virtual void         GetMemoryUsage(ICrySizer* s) const
	{
		s->Add(*this);
	}
};

//////////////////////////////////////////////////////////////////////////
// Executes an Action
//////////////////////////////////////////////////////////////////////////
class CFlowNode_AIExecute
	: public CFlowNode_AIForceableBase<true>
{
	typedef CFlowNode_AIForceableBase<true> TBase;
public:
	CFlowNode_AIExecute(IFlowNode::SActivationInfo* pActInfo) : TBase(pActInfo), m_bCancel(false) {}
	virtual void         GetConfiguration(SFlowNodeConfig& config);
	virtual void         DoProcessEvent(IFlowNode::EFlowEvent event, IFlowNode::SActivationInfo* pActInfo);
	virtual IFlowNodePtr Clone(SActivationInfo* pActInfo);
	virtual EForceMethod GetForceMethod(IFlowNode::SActivationInfo* pActInfo) const;

	virtual void         GetMemoryUsage(ICrySizer* s) const
	{
		s->Add(*this);
	}

protected:
	// should call DoProcessEvent if owner is not too much alerted
	virtual void TryExecute(IAIObject* pAI, EFlowEvent event, SActivationInfo* pActInfo);

	bool m_bCancel;
	virtual void OnCancelPortActivated(IPipeUser* pPipeUser, SActivationInfo* pActInfo);

	virtual void Cancel();
	virtual void Finish();
};

#ifdef USE_DEPRECATED_AI_CHARACTER_SYSTEM
//////////////////////////////////////////////////////////////////////////
// Set Character
//////////////////////////////////////////////////////////////////////////
class CFlowNode_AISetCharacter
	: public CFlowNode_AIBase<false>
{
	typedef CFlowNode_AIBase<false> TBase;
public:
	CFlowNode_AISetCharacter(IFlowNode::SActivationInfo* pActInfo) : TBase(pActInfo) {}
	virtual void         GetConfiguration(SFlowNodeConfig& config);
	virtual void         DoProcessEvent(IFlowNode::EFlowEvent event, IFlowNode::SActivationInfo* pActInfo);
	virtual IFlowNodePtr Clone(SActivationInfo* pActInfo);

	virtual void         GetMemoryUsage(ICrySizer* s) const
	{
		s->Add(*this);
	}
};
#endif

//////////////////////////////////////////////////////////////////////////
// Set State
//////////////////////////////////////////////////////////////////////////
class CFlowNode_AISetState
	: public CFlowNode_AIBase<false>
{
	typedef CFlowNode_AIBase<false> TBase;
public:
	CFlowNode_AISetState(IFlowNode::SActivationInfo* pActInfo) : TBase(pActInfo) {}
	virtual void         GetConfiguration(SFlowNodeConfig& config);
	virtual void         DoProcessEvent(IFlowNode::EFlowEvent event, IFlowNode::SActivationInfo* pActInfo);
	virtual IFlowNodePtr Clone(SActivationInfo* pActInfo);

	virtual void         GetMemoryUsage(ICrySizer* s) const
	{
		s->Add(*this);
	}
};

//////////////////////////////////////////////////////////////////////////
// Vehicle Follow Path
//////////////////////////////////////////////////////////////////////////
class CFlowNode_AIVehicleFollowPath
	: public CFlowNode_AIForceableBase<true>
{
	typedef CFlowNode_AIForceableBase<true> TBase;
public:
	CFlowNode_AIVehicleFollowPath(IFlowNode::SActivationInfo* pActInfo) : TBase(pActInfo) {}
	virtual void         GetConfiguration(SFlowNodeConfig& config);
	virtual void         DoProcessEvent(IFlowNode::EFlowEvent event, IFlowNode::SActivationInfo* pActInfo);
	virtual void         ProcessEvent(IFlowNode::EFlowEvent event, IFlowNode::SActivationInfo* pActInfo);
	virtual EForceMethod GetForceMethod(IFlowNode::SActivationInfo* pActInfo) const;

	virtual IFlowNodePtr Clone(SActivationInfo* pActInfo);

	virtual void         GetMemoryUsage(ICrySizer* s) const
	{
		s->Add(*this);
	}
};

//////////////////////////////////////////////////////////////////////////
// Vehicle Chase Target
//////////////////////////////////////////////////////////////////////////
class CFlowNode_AIVehicleChaseTarget
	: public CFlowNode_AIForceableBase<true>
{
	typedef CFlowNode_AIForceableBase<true> TBase;

	enum Outputs
	{
		eOut_Failed,
	};

	TFlowNodeId m_ID;
	Outputs     m_outputToActivate;
	bool        m_activateOutput;

public:
	CFlowNode_AIVehicleChaseTarget(IFlowNode::SActivationInfo* pActInfo) : TBase(pActInfo), m_outputToActivate(eOut_Failed), m_activateOutput(false) {}
	virtual void         GetConfiguration(SFlowNodeConfig& config);
	virtual void         DoProcessEvent(IFlowNode::EFlowEvent event, IFlowNode::SActivationInfo* pActInfo);
	virtual void         ProcessEvent(IFlowNode::EFlowEvent event, IFlowNode::SActivationInfo* pActInfo);
	virtual EForceMethod GetForceMethod(IFlowNode::SActivationInfo* pActInfo) const;
	virtual void         PreExecute(SActivationInfo* pActInfo);

	void                 OnEntityEvent(IEntity* pEntity, SEntityEvent& event);

	virtual IFlowNodePtr Clone(SActivationInfo* pActInfo);

	virtual void         GetMemoryUsage(ICrySizer* s) const
	{
		s->Add(*this);
	}
};

//////////////////////////////////////////////////////////////////////////
// Vehicle Stick Path
//////////////////////////////////////////////////////////////////////////
class CFlowNode_AIVehicleStickPath
	: public CFlowNode_AIForceableBase<true>
{
	typedef CFlowNode_AIForceableBase<true> TBase;

	enum Outputs
	{
		eOut_Done = 0,
		eOut_Succeeded,
		eOut_Failed,
		eOut_Close,
	};

	TFlowNodeId m_ID;
	Outputs     m_outputToActivate;
	EntityId    m_entityRegisteredToAsListener;
	bool        m_activateOutput;

public:
	~CFlowNode_AIVehicleStickPath();
	CFlowNode_AIVehicleStickPath(IFlowNode::SActivationInfo* pActInfo)
		: TBase(pActInfo)
		, m_activateOutput(false)
		, m_outputToActivate(eOut_Succeeded)
		, m_entityRegisteredToAsListener(0)
	{
	}
	virtual void         GetConfiguration(SFlowNodeConfig& config);
	virtual void         DoProcessEvent(IFlowNode::EFlowEvent event, IFlowNode::SActivationInfo* pActInfo);
	virtual void         ProcessEvent(IFlowNode::EFlowEvent event, IFlowNode::SActivationInfo* pActInfo);
	virtual EForceMethod GetForceMethod(IFlowNode::SActivationInfo* pActInfo) const;
	virtual void         PreExecute(SActivationInfo* pActInfo);

	void                 OnEntityEvent(IEntity* pEntity, SEntityEvent& event);

	virtual IFlowNodePtr Clone(SActivationInfo* pActInfo);
	virtual void         Serialize(SActivationInfo*, TSerialize ser);

	virtual void         GetMemoryUsage(ICrySizer* s) const
	{
		s->Add(*this);
	}

private:
	void RegisterForScriptEvent();
	void UnregisterFromScriptEvent();
};

//////////////////////////////////////////////////////////////////////////
// Look At
//////////////////////////////////////////////////////////////////////////
class CFlowNode_AILookAt
	: public CFlowNode_AIForceableBase<true>
{
	typedef CFlowNode_AIForceableBase<true> TBase;
	CTimeValue               m_startTime;
	bool                     m_bExecuted;
	IPipeUser::LookTargetPtr m_lookTarget;

public:
	CFlowNode_AILookAt(IFlowNode::SActivationInfo* pActInfo) : TBase(pActInfo), m_bExecuted(false) {}
	virtual void         GetConfiguration(SFlowNodeConfig& config);
	virtual void         DoProcessEvent(IFlowNode::EFlowEvent event, IFlowNode::SActivationInfo* pActInfo);
	virtual EForceMethod GetForceMethod(IFlowNode::SActivationInfo* pActInfo) const;
	virtual void         OnCancel();
	virtual void         OnFinish();
	;

	void                 ClearLookTarget();
	virtual bool         OnUpdate(IFlowNode::SActivationInfo* pActInfo);

	void                 SetLookTarget(IAIObject* pAI, Vec3 pos);
	virtual IFlowNodePtr Clone(SActivationInfo* pActInfo);

	virtual void         GetMemoryUsage(ICrySizer* s) const
	{
		s->Add(*this);
	}

private:
	virtual void PreExecute(SActivationInfo* pActInfo) { m_bExecuted = false; }
};

//////////////////////////////////////////////////////////////////////////
// body stance controller
//////////////////////////////////////////////////////////////////////////
class CFlowNode_AIStance
	: public CFlowNode_AIBase<false>
{
	typedef CFlowNode_AIBase<false> TBase;
public:
	CFlowNode_AIStance(IFlowNode::SActivationInfo* pActInfo) : TBase(pActInfo) {}
	virtual void         GetConfiguration(SFlowNodeConfig& config);
	virtual void         DoProcessEvent(IFlowNode::EFlowEvent event, IFlowNode::SActivationInfo* pActInfo);
	virtual IFlowNodePtr Clone(SActivationInfo* pActInfo);

	virtual void         GetMemoryUsage(ICrySizer* s) const
	{
		s->Add(*this);
	}
};

//////////////////////////////////////////////////////////////////////////
// unload vehicle
//////////////////////////////////////////////////////////////////////////
class CFlowNode_AIUnload
	: public CFlowNode_AIBase<true>
{
	typedef CFlowNode_AIBase<true> TBase;
	// must implement this abstract function, not called
	virtual void DoProcessEvent(IFlowNode::EFlowEvent event, IFlowNode::SActivationInfo* pActInfo);

protected:
	int                     m_numPassengers;
	std::map<int, EntityId> m_mapPassengers;

	void         UnloadSeat(IVehicle* pVehicle, int seatId);
	virtual void OnGoalPipeEvent(IPipeUser* pPipeUser, EGoalPipeEvent event, int goalPipeId, bool& unregisterListenerAfterEvent);
	virtual void UnregisterEvents();

public:
	CFlowNode_AIUnload(IFlowNode::SActivationInfo* pActInfo) : TBase(pActInfo), m_numPassengers(0) {}
	virtual ~CFlowNode_AIUnload() { UnregisterEvents(); }

	virtual void         GetConfiguration(SFlowNodeConfig& config);
	virtual void         ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo);
	virtual IFlowNodePtr Clone(SActivationInfo* pActInfo);

	virtual void         GetMemoryUsage(ICrySizer* s) const
	{
		s->Add(*this);
	}
};

//////////////////////////////////////////////////////////////////////////
// drop object
//////////////////////////////////////////////////////////////////////////
class CFlowNode_AIDropObject
	: public CFlowNode_AIForceableBase<true>
{
	typedef CFlowNode_AIForceableBase<true> TBase;
public:
	CFlowNode_AIDropObject(IFlowNode::SActivationInfo* pActInfo) : TBase(pActInfo) {}
	virtual void         GetConfiguration(SFlowNodeConfig& config);
	virtual void         DoProcessEvent(IFlowNode::EFlowEvent event, IFlowNode::SActivationInfo* pActInfo);
	virtual EForceMethod GetForceMethod(IFlowNode::SActivationInfo* pActInfo) const;
	virtual IFlowNodePtr Clone(SActivationInfo* pActInfo);

	virtual void         GetMemoryUsage(ICrySizer* s) const
	{
		s->Add(*this);
	}
};

//////////////////////////////////////////////////////////////////////////
// Uses an object
//////////////////////////////////////////////////////////////////////////
class CFlowNode_AIUseObject
	: public CFlowNode_AIForceableBase<true>
{
	typedef CFlowNode_AIForceableBase<true> TBase;
public:
	CFlowNode_AIUseObject(IFlowNode::SActivationInfo* pActInfo) : TBase(pActInfo) {}
	virtual void         GetConfiguration(SFlowNodeConfig& config);
	virtual void         DoProcessEvent(IFlowNode::EFlowEvent event, IFlowNode::SActivationInfo* pActInfo);
	virtual EForceMethod GetForceMethod(IFlowNode::SActivationInfo* pActInfo) const;
	virtual IFlowNodePtr Clone(SActivationInfo* pActInfo);

	virtual void         GetMemoryUsage(ICrySizer* s) const
	{
		s->Add(*this);
	}
};

//////////////////////////////////////////////////////////////////////////
// Makes ai enter specifyed seat of specifyed vehicle
//////////////////////////////////////////////////////////////////////////
class CFlowNode_AIEnterVehicle : public CFlowNode_AIForceableBase<true>
{
	typedef CFlowNode_AIForceableBase<true> TBase;
public:
	enum EInputs
	{
		eIn_Sync,
		eIn_Cancel,
		eIn_VehicleId,
		eIn_Seat,
		eIn_Fast,
		eIn_ForceMethod,
	};
	enum EOutputs
	{
		eOut_Done,
		eOut_Success,
		eOut_Fail
	};

	CFlowNode_AIEnterVehicle(IFlowNode::SActivationInfo* pActInfo) : CFlowNode_AIForceableBase<true>(pActInfo) {}
	virtual void         GetConfiguration(SFlowNodeConfig& config);
	virtual void         DoProcessEvent(IFlowNode::EFlowEvent event, IFlowNode::SActivationInfo* pActInfo);
	virtual EForceMethod GetForceMethod(IFlowNode::SActivationInfo* pActInfo) const;
	virtual IFlowNodePtr Clone(SActivationInfo* pActInfo);

	virtual void         GetMemoryUsage(ICrySizer* s) const
	{
		s->Add(*this);
	}
};

//////////////////////////////////////////////////////////////////////////
// Gets vehicle seat helper positions
//////////////////////////////////////////////////////////////////////////
class CFlowNode_GetSeatHelperVehicle : public CFlowVehicleBase
{
public:
	enum EInputs
	{
		eIn_Get,
		eIn_Seat,
	};
	enum EOutputs
	{
		eOut_Pos,
		eOut_Dir,
	};

	CFlowNode_GetSeatHelperVehicle(SActivationInfo* pActivationInfo)
	{
		Init(pActivationInfo);
	}
	~CFlowNode_GetSeatHelperVehicle() { Delete(); }
	IFlowNodePtr Clone(SActivationInfo* pActInfo)
	{
		IFlowNode* pNode = new CFlowNode_GetSeatHelperVehicle(pActInfo);
		return pNode;
	}
	virtual void GetConfiguration(SFlowNodeConfig& config);
	virtual void ProcessEvent(IFlowNode::EFlowEvent event, IFlowNode::SActivationInfo* pActInfo);

	virtual void GetMemoryUsage(ICrySizer* s) const
	{
		s->Add(*this);
	}
};

#endif // __FlowNodeAIAction_H__
