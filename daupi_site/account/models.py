from django.db import models
from django.contrib.auth.models import User
import uuid


class Friendship(models.Model):
    STATUS_CHOICES = [
        ('pending', 'Pending'),
        ('accepted', 'Accepted'),
        ('rejected', 'Rejected'),
    ]

    from_user = models.ForeignKey(User, related_name='friends_sent', on_delete=models.CASCADE)
    to_user = models.ForeignKey(User, related_name='friends_received', on_delete=models.CASCADE)
    status = models.CharField(max_length=10, choices=STATUS_CHOICES, default='pending')
    created_at = models.DateTimeField(auto_now_add=True)

    def __str__(self):
        return f"{self.from_user} -> {self.to_user} ({self.status})"

class Message(models.Model):
    sender = models.ForeignKey(User, on_delete=models.CASCADE, related_name="sent_messages")
    receiver = models.ForeignKey(User, on_delete=models.CASCADE, related_name="received_messages")
    content = models.TextField()
    timestamp = models.DateTimeField(auto_now_add=True)


# Модель Канала
class Channel(models.Model):
    name = models.CharField(max_length=255)
    description = models.TextField()
    creator = models.ForeignKey(User, on_delete=models.CASCADE)
    created_at = models.DateTimeField(auto_now_add=True)

    def __str__(self):
        return self.name

# Модель Подканала
class Subchannel(models.Model):
    TEXT = 'text'
    VOICE = 'voice'
    SUBCHANNEL_TYPES = [
        (TEXT, 'Text'),
        (VOICE, 'Voice'),
    ]

    channel = models.ForeignKey(Channel, on_delete=models.CASCADE, related_name='subchannels')
    name = models.CharField(max_length=255)
    type = models.CharField(max_length=10, choices=SUBCHANNEL_TYPES)
    uuid = models.UUIDField(default=uuid.uuid4, editable=False, unique=True)  # Уникальный ID подканала
    created_at = models.DateTimeField(auto_now_add=True)

    def __str__(self):
        return f"{self.name} ({self.type})"

class ChannelMember(models.Model):
    channel = models.ForeignKey(Channel, on_delete=models.CASCADE, related_name="members")
    user = models.ForeignKey(User, on_delete=models.CASCADE, related_name="channel_memberships")
    joined_at = models.DateTimeField(auto_now_add=True)

    def add_member(self, user):
        ChannelMember.objects.get_or_create(channel=self, user=user)

    class Meta:
        unique_together = ('channel', 'user')

