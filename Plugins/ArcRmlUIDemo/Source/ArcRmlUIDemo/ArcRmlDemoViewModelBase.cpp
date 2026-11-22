// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlDemoViewModelBase.h"

#include "ArcRmlDemoSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ArcRmlDemoViewModelBase)

UArcRmlDemoSubsystem* UArcRmlDemoViewModelBase::GetDemo() const
{
	return GetTypedOuter<UArcRmlDemoSubsystem>();
}
