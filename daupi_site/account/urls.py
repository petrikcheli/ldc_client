from django.urls import path
from django.contrib.auth import views as auth_views
from . import views
from rest_framework_simplejwt.views import TokenObtainPairView, TokenRefreshView
from .views import *


urlpatterns = [
    #path('login/', views.user_login, name='login'),
    # url-адреса входа и выхода
    path('login/', auth_views.LoginView.as_view(), name='login'),
    path('logout/', auth_views.LogoutView.as_view(), name='logout'),   

     # url-адреса сброса пароля
    path('password-reset/',auth_views.PasswordResetView.as_view(),name='password_reset'),
    path('password-reset/done/',auth_views.PasswordResetDoneView.as_view(),name='password_reset_done'),

    path('password-reset/',auth_views.PasswordResetView.as_view(),name='password_reset'),
    path('password-reset/done/',auth_views.PasswordResetDoneView.as_view(),name='password_reset_done'),
    # path('password-reset/<uidb64>/<token>/',auth_views.PasswordResetConfirmView.as_view(),name='password_reset_confirm'),
    # path('password-reset/complete/',auth_views.PasswordResetCompleteView.as_view(),name='password_reset_complete'),

    path('register/', views.register, name='register'),
    path('search/', views.search_users, name='search_users'),  # маршрут для поиска

    path('send_friend_request/', views.send_friend_request, name='send_friend_request'),
    path('friend_requests/', views.friend_requests, name='friend_requests'),
    path('respond_to_friend_request/<int:request_id>/<str:response>/', views.respond_to_friend_request, name='respond_to_friend_request'),

    path('friends/', views.user_friends, name='friends'),
    #path("create-channel/", create_channel_view, name="create-channel"),
    path("channel/<int:channel_id>/create-subchannel/", create_subchannel_view, name="create-subchannel"),
    path("channel/<int:channel_id>/invite/", invite_to_channel_view, name="invite-to-channel"),
    path("my-channels/", views.my_channels_view, name="my-channels"),
    path('channel/create/', views.create_channel_view, name='create-channel'),
    path('channel/<int:channel_id>/delete/', views.delete_channel_view, name='channel_delete'),
    path('account/channel/<int:channel_id>/', views.channel_detail_view, name='channel_detail'),
    path('account/channel/<int:channel_id>/create-subchannel/', views.create_subchannel_view, name='create_subchannel'),


    path('api/register/', RegisterView.as_view(), name='api-register'),
    path('api/login/', TokenObtainPairView.as_view(), name='api-login'),
    path('api/token/refresh/', TokenRefreshView.as_view(), name='token-refresh'),
    path('api/search/', UserSearchView.as_view(), name='api-user-search'),
    path('api/friend-request/', SendFriendRequestView.as_view(), name='api-friend-request'),
    path('api/friends/', FriendsListView.as_view(), name='api-friends-list'),

    path('api/token/', TokenObtainPairView.as_view(), name='api-token_obtain_pair'),
    path('api/token/refresh/', TokenRefreshView.as_view(), name='api-token_refresh'),

    path('api/messages/<int:user_id>/', MessageListView.as_view(), name='api-messages'),
    path('api/send-message/', SendMessageView.as_view(), name='api-send-message'),
    path('api/messages/with/<str:username>/', MessageHistoryView.as_view(), name='api-message-history'),

    path('api/channels/', ChannelListView.as_view(), name='api-channel-list'),
    path('api/channels/<int:channel_id>/subchannels/', SubchannelListView.as_view(), name='api-subchannel-list'),
    path('api/channels/create/', CreateChannelView.as_view(), name='api-create-channel'),
    path('api/channels/<int:channel_id>/subchannels/create/', CreateSubchannelView.as_view(), name='api-create-subchannel'),

    path('', views.dashboard, name='dashboard'), 
]