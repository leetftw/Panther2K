#pragma once
#include "Control.h"

namespace Leet::LibPanther::Layout
{
	class SpacerControl : public Control
	{
	public:
		SpacerControl(int height);
		void Draw(Leet::Panther2K::Util::Console* console) override;
	};
}