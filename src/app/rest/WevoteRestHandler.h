#ifndef WEVOTERESTHANDLER_H
#define WEVOTERESTHANDLER_H

#include "RestHandler.h"
#include "WevoteRestMessages.hpp"

namespace wevote
{
namespace web
{
class WevoteRestHandler : public RestHandler
{
public:
    explicit WevoteRestHandler( utility::string_t url );
protected:
    void _addRoutes() override;
private:
    void _submitWevoteEnsemble(http_request message);
};

}
}

#endif // WEVOTERESTHANDLER_H