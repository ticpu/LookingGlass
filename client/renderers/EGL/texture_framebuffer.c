/**
 * Looking Glass
 * Copyright © 2017-2021 The Looking Glass Authors
 * https://looking-glass.io
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc., 59
 * Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include "texture.h"

#include <assert.h>

#include "texture_buffer.h"
#include "common/debug.h"
#include "common/KVMFR.h"
#include "common/rects.h"

struct TexDamage
{
  int             count;
  FrameDamageRect rects[KVMFR_MAX_DAMAGE_RECTS];
};

typedef struct TexFB
{
  TextureBuffer base;
  struct TexDamage damage[EGL_TEX_BUFFER_MAX];
}
TexFB;

static bool eglTexFB_init(EGL_Texture ** texture, EGLDisplay * display)
{
  TexFB * this = calloc(sizeof(*this), 1);
  *texture = &this->base.base;

  EGL_Texture * parent = &this->base.base;
  if (!eglTexBuffer_stream_init(&parent, display))
  {
    free(this);
    *texture = NULL;
    return false;
  }

  for (int i = 0; i < EGL_TEX_BUFFER_MAX; ++i)
    this->damage[i].count = -1;

  return true;
}

static bool eglTexFB_update(EGL_Texture * texture, const EGL_TexUpdate * update)
{
  TextureBuffer * parent = UPCAST(TextureBuffer, texture);
  TexFB         * this   = UPCAST(TexFB        , parent );

  assert(update->type == EGL_TEXTYPE_FRAMEBUFFER);

  LG_LOCK(parent->copyLock);

  struct TexDamage * damage = this->damage + parent->bufIndex;
  bool damageAll = !update->rects || update->rectCount == 0 || damage->count < 0 ||
    damage->count + update->rectCount > KVMFR_MAX_DAMAGE_RECTS;

  if (damageAll)
    framebuffer_read(
      update->frame,
      parent->buf[parent->bufIndex].map,
      parent->format.stride,
      parent->format.height,
      parent->format.width,
      parent->format.bpp,
      parent->format.stride
    );
  else
  {
    memcpy(damage->rects + damage->count, update->rects,
      update->rectCount * sizeof(FrameDamageRect));
    damage->count += update->rectCount;
    rectsFramebufferToBuffer(
      damage->rects,
      damage->count,
      parent->buf[parent->bufIndex].map,
      parent->format.stride,
      parent->format.height,
      update->frame,
      parent->format.stride
    );
  }

  parent->buf[parent->bufIndex].updated = true;

  for (int i = 0; i < EGL_TEX_BUFFER_MAX; ++i)
  {
    struct TexDamage * damage = this->damage + i;
    if (i == parent->bufIndex)
      damage->count = 0;
    else if (update->rects && update->rectCount > 0 && damage->count >= 0 &&
             damage->count + update->rectCount <= KVMFR_MAX_DAMAGE_RECTS)
    {
      memcpy(damage->rects + damage->count, update->rects,
        update->rectCount * sizeof(FrameDamageRect));
      damage->count += update->rectCount;
    }
    else
      damage->count = -1;
  }

  LG_UNLOCK(parent->copyLock);

  return true;
}

EGL_TextureOps EGL_TextureFrameBuffer =
{
  .init        = eglTexFB_init,
  .free        = eglTexBuffer_free,
  .setup       = eglTexBuffer_stream_setup,
  .update      = eglTexFB_update,
  .process     = eglTexBuffer_stream_process,
  .bind        = eglTexBuffer_stream_bind
};
