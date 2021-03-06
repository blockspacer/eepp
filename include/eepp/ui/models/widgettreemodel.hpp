#ifndef EE_UI_MODELS_WIDGETTREEMODEL_HPP
#define EE_UI_MODELS_WIDGETTREEMODEL_HPP

#include <eepp/scene/node.hpp>
#include <eepp/ui/models/model.hpp>
#include <memory>

using namespace EE::UI;

namespace EE { namespace UI { namespace Models {

class EE_API WidgetTreeModel : public Model {
  public:
	static std::shared_ptr<WidgetTreeModel> New( Node* node );

	virtual size_t rowCount( const ModelIndex& = ModelIndex() ) const override;

	virtual size_t columnCount( const ModelIndex& = ModelIndex() ) const override;

	virtual Variant data( const ModelIndex&, Role = Role::Display ) const override;

	virtual ModelIndex index( int row, int column,
							  const ModelIndex& parent = ModelIndex() ) const override;

	virtual ModelIndex parentIndex( const ModelIndex& ) const override;

	virtual void update() override;

  protected:
	Node* mRoot;

	WidgetTreeModel( Node* node );
};

}}} // namespace EE::UI::Models

#endif // EE_UI_MODELS_WIDGETTREEMODEL_HPP
