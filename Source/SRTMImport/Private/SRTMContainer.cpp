#include "SRTMContainer.h"

int32 USRTMContainer::Get(int32 X, int32 Y) const
{
	return (int32)Get_Raw(X, Y);
}

int16 USRTMContainer::Get_Raw(int32 X, int32 Y) const
{
	int32 DataIdx = X + (Y * DimensionSize.Y);

	if (!RawData.IsValidIndex(DataIdx)) return 0;

	return RawData[DataIdx];
}