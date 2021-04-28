; (function ($) {
    "use strict";

    if (!window.Origin) { window.Origin = {}; }
    if (!Origin.views) { Origin.views = {}; }

    var NetPromoterView = function ()
    {
        var entitlement = null,
            configuredUrls = null,
            boxart = null,
            iUrl = 0,
            displayName = '';
        this.offerId = $.getQueryString("offerId");
        if (!this.offerId)
        {
            this.offerId = "long";
        };
        entitlement = entitlementManager.getEntitlementByProductId(this.offerId);
        displayName = entitlement.title;
        $('html').addClass(Origin.views.currentPlatform());

        onlineStatus.onlineStateChanged.connect($.proxy(NetPromoterView.prototype, 'onOnlineStatusChange'));

        configuredUrls = entitlement.boxartUrls;
        if(configuredUrls.length === 0 && entitlement.parent)
            configuredUrls = entitlement.parent.boxartUrls;

        for (iUrl = 0; iUrl < configuredUrls.length; iUrl++)
        {
            if (Origin.BadImageCache.shouldTryUrl(configuredUrls[iUrl]))
            {
                boxart = configuredUrls[iUrl];
                break;
            }
        }

        $('.boxart').error(function() {
                // There was an error loading the box art... set to default.
                $('.boxart').attr("src", "./netpromoter/images/default-boxart.jpg");
            }.bind(this));
        $('.boxart').attr("src", boxart);

        $('.title').text(tr('ebisu_client_game_nps_title_caps').replace("%1", displayName.toUpperCase()));
        $('#lblRadioRating').text(tr('ebisu_client_game_nps_how_likely_are_you_to_recommend_game').replace("%1", displayName));
        var $radiorating = $.otkRadioRatingCreate($('#radio-rating'),
            {
                lessLabel: tr("ebisu_client_not_at_all"),
                moreLabel: tr("ebisu_client_extremely_likely"),
                name: "radio-rating"
            });
        $radiorating.click(function() {
            NetPromoterView.prototype.refreshSubmitButton();
        });

        $('#btnSubmit').click(function() {
            if($('#btnSubmit').hasClass('disabled') === false)
            {
                telemetryClient.sendNetPromoterResults(
                    this.offerId,
                    $('#radio-rating').otkRadioRating('value'),
                    $('#chkSendFeedback').is(':checked'),
                    $('.multiline').text()
                );
                window.location.href = './confirmation.html';
            }
        }.bind(this));

        $('#btnCancel').click(function() {
            if($('#btnCancel').hasClass('disabled') === false)
            {
                telemetryClient.sendNetPromoterResults(this.offerId, "-1", false,"");
                helper.closeWindow();
            }
        }.bind(this));
    };

    NetPromoterView.prototype.onOnlineStatusChange = function ()
    {
        NetPromoterView.prototype.refreshSubmitButton();
    };

    NetPromoterView.prototype.refreshSubmitButton = function ()
    {
        var isEnabled = onlineStatus.onlineState && $('#radio-rating').otkRadioRating('value') !== -1;
        $('#btnSubmit').toggleClass('disabled', isEnabled === false);
    };

    // Expose to the world
    Origin.views.netpromoter = new NetPromoterView();

})(jQuery);