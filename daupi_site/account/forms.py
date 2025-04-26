from django import forms
from .models import *
from django.contrib.auth.models import User


class LoginForm(forms.Form):
    username = forms.CharField()
    password = forms.CharField(widget=forms.PasswordInput)

class UserRegistrationForm(forms.ModelForm):
    password = forms.CharField(label='Password',
                                widget=forms.PasswordInput)
    password2 = forms.CharField(label='Repeat password',
                                widget=forms.PasswordInput)
    class Meta:
        model = User
        fields = ['username', 'first_name', 'email']    

    def clean_password2(self):
        cd = self.cleaned_data
        if cd['password'] != cd['password2']:
            raise forms.ValidationError('Passwords don\'t match.')
        return cd['password2']
    
class UserSearchForm(forms.Form):
    query = forms.CharField(max_length=100, required=False, label='Search by username')


class FriendshipRequestForm(forms.Form):
    to_user = forms.ModelChoiceField(queryset=User.objects.all(), label='Select friend', required=True)

class ChannelForm(forms.ModelForm):
    class Meta:
        model = Channel
        fields = ['name']  # укажи нужные поля

class SubchannelForm(forms.ModelForm):
    class Meta:
        model = Subchannel
        fields = ['name', 'type']  # Только имя и тип (текстовый/голосовой)