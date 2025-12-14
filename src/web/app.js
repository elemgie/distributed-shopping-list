(function () {
  const form = document.getElementById('item-form');
  const nameInput = document.getElementById('name');
  const desiredInput = document.getElementById('desired');
  const currentInput = document.getElementById('current');
  const itemsEl = document.getElementById('items');
  const clearFormBtn = document.getElementById('clear-form');
  const listNameInput = document.getElementById('list-name');
  const listUidInput = document.getElementById('list-uid');
  const loadListBtn = document.getElementById('load-list');
  const createListBtn = document.getElementById('create-list');
  const currentListEl = document.getElementById('current-list');
  const deleteListBtn = document.getElementById('delete-list');

  let currentListId = '';

  const serverBaseInput = document.getElementById('server-base');

  let serverBase = (localStorage.getItem('serverBase')) || 'http://localhost:9080';
  if (serverBaseInput) {
    serverBaseInput.value = serverBase;
    serverBaseInput.addEventListener('change', () => {
      serverBase = (serverBaseInput.value || '').trim() || 'http://localhost:9080';
      localStorage.setItem('serverBase', serverBase);
    });
  }

  async function apiCreateList(name) {
    const res = await fetch(`${serverBase}/shopping_list`, {
      method: 'PUT',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ name })
    });
    if (!res.ok) throw new Error('Failed to create list');
    return res.json();
  }

  async function apiGetList(id) {
    const res = await fetch(`${serverBase}/shopping_list/${encodeURIComponent(id)}`);
    if (res.status === 404) throw new Error('List not found');
    if (!res.ok) throw new Error('Failed to fetch list');
    return res.json();
  }

  async function apiDeleteList(id) {
    const res = await fetch(`${serverBase}/shopping_list/${encodeURIComponent(id)}`, { method: 'DELETE' });
    if (!res.ok) throw new Error('Failed to delete list');
  }

  function makeItemBody(name, desired, current) {
    return {
      name: name || '',
      desired_quantity: Number(desired) || 0,
      current_quantity: Number(current) || 0
    };
  }

  async function apiAddItem(listId, body) {
    const res = await fetch(`${serverBase}/shopping_list/${encodeURIComponent(listId)}/item`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(body)
    });
    if (!res.ok) throw new Error('Failed to add item');
    return res.json();
  }

  async function apiUpdateItem(listId, itemId, body) {
    const res = await fetch(`${serverBase}/shopping_list/${encodeURIComponent(listId)}/item/${encodeURIComponent(itemId)}`, {
      method: 'PUT',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(body)
    });
    if (!res.ok) throw new Error('Failed to update item');
    return res.json();
  }

  async function apiDeleteItem(listId, itemId) {
    const res = await fetch(`${serverBase}/shopping_list/${encodeURIComponent(listId)}/item/${encodeURIComponent(itemId)}`, {
      method: 'DELETE'
    });
    if (!res.ok) throw new Error('Failed to delete item');
    return res.json();
  }

  function renderEmpty(msg) {
    itemsEl.innerHTML = '';
    const li = document.createElement('li');
    li.className = 'empty';
    li.textContent = msg;
    itemsEl.appendChild(li);
  }

  function renderItems(list) {
    itemsEl.innerHTML = '';
    if (!list.items || list.items.length === 0) {
      renderEmpty('No items yet.');
      return;
    }
    list.items.forEach((it) => {
      const li = document.createElement('li');
      li.className = 'item';
      li.dataset.uid = it.uid;
      const title = document.createElement('div');
      title.className = 'title';
      const desired = it.desiredQuantity ?? it.desired_quantity;
      const current = it.currentQuantity ?? it.current_quantity;
      title.textContent = `${it.name} (desired: ${desired}, current: ${current})`;

      const actions = document.createElement('div');
      actions.className = 'actions';
      const editBtn = document.createElement('button');
      editBtn.textContent = 'Edit';
      editBtn.addEventListener('click', () => populateFormForEdit(it));
      const delBtn = document.createElement('button');
      delBtn.textContent = 'Delete';
      delBtn.addEventListener('click', () => deleteItem(it.uid));
      actions.appendChild(editBtn);
      actions.appendChild(delBtn);

      li.appendChild(title);
      li.appendChild(actions);
      itemsEl.appendChild(li);
    });
  }

  function populateFormForEdit(it) {
    nameInput.value = it.name || '';
    const desired = it.desiredQuantity ?? it.desired_quantity;
    const current = it.currentQuantity ?? it.current_quantity;
    desiredInput.value = desired ?? 0;
    currentInput.value = current ?? 0;
    form.dataset.editUid = it.uid;
  }

  async function refreshList() {
    if (!currentListId) {
      renderEmpty('No list loaded.');
      return;
    }
    try {
      const list = await apiGetList(currentListId);
      listNameInput.value = list.name || listNameInput.value;
      currentListEl.textContent = `${list.name || currentListId} (${currentListId})`;
      renderItems(list);
    } catch (err) {
      console.error(err);
      renderEmpty('Failed to load list.');
    }
  }

  function ensureListLoaded() {
    if (!currentListId) {
      alert('No list selected. Create or Load a list first.');
      return false;
    }
    return true;
  }

  async function addOrUpdate() {
    if (!ensureListLoaded()) return;
    const itemName = nameInput.value.trim();
    const desired = desiredInput.value;
    const current = currentInput.value;
    const editUid = form.dataset.editUid;

    try {
      if (editUid) {
        await apiUpdateItem(currentListId, editUid, makeItemBody(itemName, desired, current));
      } else {
        await apiAddItem(currentListId, makeItemBody(itemName, desired, current));
      }
      delete form.dataset.editUid;
      form.reset();
      await refreshList();
    } catch (err) {
      alert('Operation failed. Check server logs.');
      console.error(err);
    }
  }

  async function deleteItem(uid) {
    if (!currentListId) return;
    if (!confirm('Delete item?')) return;
    try {
      await apiDeleteItem(currentListId, uid);
      await refreshList();
    } catch (err) {
      alert('Delete failed.');
      console.error(err);
    }
  }

  form.addEventListener('submit', (e) => { e.preventDefault(); addOrUpdate(); });
  clearFormBtn.addEventListener('click', () => { form.reset(); delete form.dataset.editUid; });
  loadListBtn.addEventListener('click', async () => {
    const uid = listUidInput.value.trim();
    if (!uid) return alert('Enter list UID to load');
    try {
      const list = await apiGetList(uid);
      currentListId = uid;

      listNameInput.value = list.name || '';
      currentListEl.textContent = `${list.name || currentListId} (${currentListId})`;
      renderItems(list);
    } catch (err) {
      alert('Failed to load list. It may not exist.');
      console.error(err);
    }
  });

  createListBtn.addEventListener('click', async () => {
    const name = listNameInput.value.trim();
    if (!name) return alert('Enter list name to create');
    try {
      const list = await apiCreateList(name);
      const uid = list.uid || list.id || '';
      currentListId = uid;
      listUidInput.value = uid;
      listNameInput.value = list.name || name;
      currentListEl.textContent = `${list.name || name} (${currentListId})`;
      renderItems(list);
    } catch (err) {
      alert('Failed to create list.');
      console.error(err);
    }
  });

  deleteListBtn.addEventListener('click', async () => {
    const uid = (listUidInput.value || currentListId).trim();
    if (!uid) return alert('No list UID provided to delete');
    if (!confirm(`Delete list "${uid}"? This cannot be undone.`)) return;
    try {
      await apiDeleteList(uid);
      currentListId = '';
      currentListEl.textContent = '(no list loaded)';
      renderEmpty('No list loaded.');
      form.reset();
      delete form.dataset.editUid;
      listNameInput.value = '';
      listUidInput.value = '';
    } catch (err) {
      alert('Failed to delete list.');
      console.error(err);
    }
  });

  renderEmpty('No list loaded.');
})();
