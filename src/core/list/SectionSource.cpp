#include "SectionSource.hpp"

#include "core/actions/ActionPanelState.hpp"

// Out-of-line so subclasses that don't override this don't force every
// including TU to see ActionPanelState's full definition (see header comment).
std::unique_ptr<ActionPanelState> SectionSource::actionsFor(int) const {
    return nullptr;
}
