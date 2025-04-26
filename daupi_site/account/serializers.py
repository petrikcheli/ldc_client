from rest_framework import serializers
from django.contrib.auth.models import User
from .models import *

class UserSerializer(serializers.ModelSerializer):
    class Meta:
        model = User
        fields = ['id', 'username', 'email']

class RegisterSerializer(serializers.ModelSerializer):
    password = serializers.CharField(write_only=True)

    class Meta:
        model = User
        fields = ['username', 'email', 'password']

    def create(self, validated_data):
        user = User.objects.create_user(
            username=validated_data['username'],
            email=validated_data['email'],
            password=validated_data['password']
        )
        return user

class FriendRequestSerializer(serializers.ModelSerializer):
    class Meta:
        model = Friendship
        fields = ['id', 'from_user', 'to_user', 'status']

class MessageSerializer(serializers.ModelSerializer):
    sender_username = serializers.CharField(source='sender.username', read_only=True)
    class Meta:
        model = Message
        fields = ['id', 'sender', 'receiver', 'sender_username', 'content', 'timestamp']
        extra_kwargs = {
            'sender': {'read_only': True}  # запретить клиенту передавать sender вручную
        }

class SubchannelSerializer(serializers.ModelSerializer):
    class Meta:
        model = Subchannel
        fields = ['id', 'name', 'type', 'uuid', 'created_at']

class ChannelSerializer(serializers.ModelSerializer):
    subchannels = SubchannelSerializer(many=True, read_only=True)

    class Meta:
        model = Channel
        fields = ['id', 'name', 'description', 'creator', 'created_at', 'subchannels']