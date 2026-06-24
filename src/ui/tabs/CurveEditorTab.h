#include "../widgets/CurveRowWidget.h"
#include <array>

class MorphSurfaceWidget;

class CurveEditorTab : public QWidget
{
  Q_OBJECT

public:
  explicit CurveEditorTab(QWidget* parent, bool isOnlyForKey);

  std::array<PiecewiseCurve, numOfCurves> curves() const;
  void setCurves(const std::array<PiecewiseCurve, numOfCurves>& curves);
  void setKeyboardRange(KeyboardRangeId kr);

private:
  std::array<CurveRowWidget*, numOfCurves> curveRows_{};
};