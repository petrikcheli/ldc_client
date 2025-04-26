from rest_framework import generics, permissions

from django.http import HttpResponse
from django.shortcuts import render, redirect
from django.contrib.auth import authenticate, login
from django.contrib.auth.decorators import login_required
from .forms import *
from django.shortcuts import get_object_or_404
from django.contrib.auth.models import User
from django.db import models 



from .serializers import *
from rest_framework.response import Response
from rest_framework.views import APIView
from rest_framework_simplejwt.tokens import RefreshToken
from django.core.exceptions import ObjectDoesNotExist


from .models import *


def user_login(request):
    if request.method == 'POST':
        form = LoginForm(request.POST)
        if form.is_valid():
            cd = form.cleaned_data
            user = authenticate(request,
                                username=cd['username'],
                                password=cd['password'])
            if user is not None:
                if user.is_active:
                    login(request, user)
                    return HttpResponse('Authenticated successfully')
                else:
                    return HttpResponse('Disabled account')
            else:
                return HttpResponse('Invalid login')
    else:
        form = LoginForm()
    return render(request, 'account/login.html', {'form': form})

@login_required
def dashboard(request):
    return render(request,
                'account/dashboard.html',
                {'section': 'dashboard'})


def register(request):
    if request.method == 'POST':
        user_form = UserRegistrationForm(request.POST)
        if user_form.is_valid():
            # Создать новый объект пользователя,
            # но пока не сохранять его
            new_user = user_form.save(commit=False)
            # Установить выбранный пароль
            new_user.set_password(user_form.cleaned_data['password'])
            # Сохранить объект User
            new_user.save()        

            return render(request,'account/register_done.html',{'new_user': new_user})
    else:
        user_form = UserRegistrationForm()
    return render(request,'account/register.html',{'user_form': user_form})

@login_required
def search_users(request):
    form = UserSearchForm(request.GET or None)
    users = []

    if form.is_valid():
        query = form.cleaned_data.get('query')
        if query:
            users = User.objects.filter(username__icontains=query)

    return render(request, 'account/search_users.html', {'form': form, 'users': users})

@login_required
def send_friend_request(request):
    if request.method == 'POST':
        form = FriendshipRequestForm(request.POST)
        if form.is_valid():
            to_user = form.cleaned_data['to_user']
            if to_user != request.user:
                # Проверяем, не отправлял ли уже текущий пользователь запрос
                existing_request = Friendship.objects.filter(
                    from_user=request.user, to_user=to_user
                ).first()
                if existing_request:
                    # Если запрос уже существует
                    return redirect('friend_requests')  # перенаправить на страницу запросов на дружбу
                # Создаём новый запрос на дружбу
                Friendship.objects.create(from_user=request.user, to_user=to_user)
                return redirect('friend_requests')  # перенаправление после успешной отправки
    else:
        form = FriendshipRequestForm()

    return render(request, 'account/send_friend_request.html', {'form': form})

@login_required
def create_channel_view(request):
    if request.method == 'POST':
        form = ChannelForm(request.POST)
        if form.is_valid():
            channel = form.save(commit=False)
            channel.creator = request.user  # Вот это ключевой момент
            channel.save()
            ChannelMember.add_member(channel, request.user)
            #return redirect('account/my_channels.html', channel_id=channel.id)
    else:
        form = ChannelForm()
    return render(request, 'account/create_channel.html', {'form': form})

@login_required
def friend_requests(request):
    # Запросы на дружбу, которые пользователь получил
    received_requests = Friendship.objects.filter(to_user=request.user, status='pending')
    return render(request, 'account/friend_requests.html', {'received_requests': received_requests})

@login_required
def respond_to_friend_request(request, request_id, response):
    try:
        friendship_request = Friendship.objects.get(id=request_id, to_user=request.user)
    except Friendship.DoesNotExist:
        # Запрос не найден
        return redirect('friend_requests')

    if response == 'accept':
        friendship_request.status = 'accepted'
    elif response == 'reject':
        friendship_request.status = 'rejected'
    friendship_request.save()

    return redirect('friend_requests')


@login_required
def user_friends(request):
    # Получаем все дружеские отношения с текущим пользователем
    friends = Friendship.objects.filter(
        models.Q(from_user=request.user, status='accepted') | 
        models.Q(to_user=request.user, status='accepted')
    )
    
    # Список друзей
    friend_list = []
    for friendship in friends:
        if friendship.from_user == request.user:
            friend_list.append(friendship.to_user)
        else:
            friend_list.append(friendship.from_user)

    return render(request, 'account/friends_list.html', {'friend_list': friend_list})



# Регистрация
class RegisterView(generics.CreateAPIView):
    queryset = User.objects.all()
    serializer_class = RegisterSerializer

# Получение токена JWT будет через встроенный view simplejwt

# Поиск пользователей
class UserSearchView(APIView):
    permission_classes = [permissions.IsAuthenticated]

    def get(self, request):
        query = request.query_params.get('query', '')
        users = User.objects.filter(username__icontains=query).exclude(id=request.user.id)
        serializer = UserSerializer(users, many=True)
        return Response(serializer.data)

# Добавление в друзья
class SendFriendRequestView(APIView):
    permission_classes = [permissions.IsAuthenticated]

    def post(self, request):
        to_user_id = request.data.get('to_user_id')
        to_user = User.objects.get(id=to_user_id)
        Friendship.objects.create(from_user=request.user, to_user=to_user)
        return Response({'status': 'Friend request sent'})

# Список друзей
class FriendsListView(APIView):
    permission_classes = [permissions.IsAuthenticated]

    def get(self, request):
        user = request.user

        # Получаем все дружеские связи, где участвует пользователь, и статус "accepted"
        friendships = Friendship.objects.filter(
            models.Q(from_user=user) | models.Q(to_user=user),
            status='accepted'
        )

        # Формируем список друзей
        friend_list = []
        for friendship in friendships:
            if friendship.from_user == user:
                friend = friendship.to_user
            else:
                friend = friendship.from_user
            friend_list.append({
                "id": friend.id,
                "username": friend.username
            })

        return Response(friend_list)
    
class MessageListView(APIView):
    permission_classes = [permissions.IsAuthenticated]

    def get(self, request, user_id):
        # Получаем все сообщения между пользователем и собеседником
        messages = Message.objects.filter(
            (models.Q(sender=request.user, receiver_id=user_id) |
             models.Q(sender_id=user_id, receiver=request.user))
        ).order_by('timestamp')
        serializer = MessageSerializer(messages, many=True)
        return Response(serializer.data)

class SendMessageView(APIView):
    permission_classes = [permissions.IsAuthenticated]

    def post(self, request):
        username = request.data.get("receiver_username")
        content = request.data.get("content")

        try:
            receiver = User.objects.get(username=username)
        except User.DoesNotExist:
            return Response({"error": "Пользователь не найден"}, status=404)

        message = Message.objects.create(
            sender=request.user,
            receiver=receiver,
            content=content
        )
        serializer = MessageSerializer(message)
        return Response(serializer.data, status=201)
    
class MessageHistoryView(APIView):
    permission_classes = [permissions.IsAuthenticated]

    def get(self, request, username):
        current_user = request.user

        try:
            other_user = User.objects.get(username=username)
        except User.DoesNotExist:
            return Response({"error": "User not found"}, status=404)

        # Все сообщения между текущим пользователем и найденным
        messages = Message.objects.filter(
            models.Q(sender=current_user, receiver=other_user) |
            models.Q(sender=other_user, receiver=current_user)
        ).order_by('timestamp')

        serializer = MessageSerializer(messages, many=True)
        return Response(serializer.data)

class ChannelListView(APIView):
    permission_classes = [permissions.IsAuthenticated]

    def get(self, request):
        user = request.user

        # находим все Channel через ChannelMember
        memberships = ChannelMember.objects.filter(user=user).select_related('channel')
        channels = [membership.channel for membership in memberships]

        serializer = ChannelSerializer(channels, many=True)
        return Response(serializer.data)


class SubchannelListView(APIView):
    permission_classes = [permissions.IsAuthenticated]

    def get(self, request, channel_id):
        """Получить все подканалы конкретного канала"""
        try:
            channel = Channel.objects.get(id=channel_id)
        except Channel.DoesNotExist:
            return Response({"error": "Channel not found"}, status=404)

        subchannels = channel.subchannels.all()
        serializer = SubchannelSerializer(subchannels, many=True)
        return Response(serializer.data)

class CreateChannelView(APIView):
    permission_classes = [permissions.IsAuthenticated]

    def post(self, request):
        """Создание нового канала"""
        name = request.data.get('name')
        description = request.data.get('description')

        if not name or not description:
            return Response({"error": "Name and description are required"}, status=400)

        channel = Channel.objects.create(
            name=name,
            description=description,
            creator=request.user
        )

        channel.users.add(request.user)  # Добавляем создателя канала в участников
        serializer = ChannelSerializer(channel)
        return Response(serializer.data, status=201)

class CreateSubchannelView(APIView):
    permission_classes = [permissions.IsAuthenticated]

    def post(self, request, channel_id):
        """Создание подканала в канале"""
        try:
            channel = Channel.objects.get(id=channel_id)
        except Channel.DoesNotExist:
            return Response({"error": "Channel not found"}, status=404)

        name = request.data.get('name')
        type = request.data.get('type')

        if not name or not type:
            return Response({"error": "Name and type are required"}, status=400)

        subchannel = Subchannel.objects.create(
            channel=channel,
            name=name,
            type=type
        )
        serializer = SubchannelSerializer(subchannel)
        return Response(serializer.data, status=201)

# @login_required
# def create_channel_view(request):
#     success = False
#     if request.method == "POST":
#         name = request.POST.get("name")
#         if name:
#             Channel.objects.create(name=name, created_by=request.user)
#             success = True
#     return render(request, "account/create_channel.html", {"success": success})

@login_required
def create_subchannel_view(request, channel_id):
    channel = get_object_or_404(Channel, id=channel_id)
    success = False

    if request.method == "POST":
        name = request.POST.get("name")
        sub_type = request.POST.get("type")
        if name and sub_type in ['text', 'voice']:
            Subchannel.objects.create(
                name=name,
                channel=channel,
                type=sub_type
            )
            success = True

    return render(request, "account/create_subchannel.html", {
        "channel": channel,
        "success": success
    })

from .models import Channel, ChannelMember
from django.contrib.auth.models import User
from django.shortcuts import get_object_or_404
from django.contrib.auth.decorators import login_required

@login_required
def invite_to_channel_view(request, channel_id):
    channel = get_object_or_404(Channel, id=channel_id)
    success = False
    error = None

    if request.method == "POST":
        username = request.POST.get("username")
        try:
            user = User.objects.get(username=username)
            if ChannelMember.objects.filter(channel=channel, user=user).exists():
                error = "Пользователь уже в канале."
            else:
                ChannelMember.objects.create(channel=channel, user=user)
                success = True
        except User.DoesNotExist:
            error = "Пользователь не найден."

    return render(request, "account/invite_to_channel.html", {
        "channel": channel,
        "success": success,
        "error": error
    })

@login_required
def my_channels_view(request):
    channels = Channel.objects.filter(members__user=request.user)
    return render(request, 'account/my_channels.html', {'channels': channels})

@login_required
def delete_channel_view(request, channel_id):
    channel = get_object_or_404(Channel, id=channel_id, creator=request.user)
    if request.method == "POST":
        channel.delete()
        return redirect('my-channels')
    return redirect('my-channels', channel_id=channel.id)

@login_required
def channel_detail_view(request, channel_id):
    channel = get_object_or_404(Channel, id=channel_id)
    subchannels = channel.subchannels.all()  # related_name='subchannels' в модели Subchannel
    return render(request, 'account/channel_detail.html', {'channel': channel, 'subchannels': subchannels})

from django import forms

class SubchannelForm(forms.ModelForm):
    class Meta:
        model = Subchannel
        fields = ['name', 'type']  # Только имя и тип (текстовый/голосовой)

@login_required
def create_subchannel_view(request, channel_id):
    channel = get_object_or_404(Channel, id=channel_id)

    # # Проверяем, что только создатель может добавлять подканалы
    # if channel.creator != request.user:
    #     return HttpResponseForbidden("Вы не можете создавать подканалы в этом канале.")

    if request.method == 'POST':
        form = SubchannelForm(request.POST)
        if form.is_valid():
            subchannel = form.save(commit=False)
            subchannel.channel = channel
            subchannel.save()
            return redirect('channel_detail', channel_id=channel.id)
    else:
        form = SubchannelForm()

    return render(request, 'account/create_subchannel.html', {'form': form, 'channel': channel})
